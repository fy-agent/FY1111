import { useRef, useState } from "react";
import { canonicalChord, modifiersFromKeyboardEvent, primaryFromKeyboardEvent } from "./validation";

export function ChordField({
  label,
  keys,
  disabled,
  onChange
}: {
  label: string;
  keys: string[];
  disabled: boolean;
  onChange: (keys: string[]) => void;
}) {
  const [listening, setListening] = useState(false);
  const [preview, setPreview] = useState<string[]>([]);
  const primariesRef = useRef<string[]>([]);
  const tokensRef = useRef<string[]>([]);

  function start(): void {
    if (disabled) return;
    primariesRef.current = [];
    tokensRef.current = [];
    setPreview([]);
    setListening(true);
  }

  function cancel(): void {
    primariesRef.current = [];
    tokensRef.current = [];
    setPreview([]);
    setListening(false);
  }

  function commit(tokens: string[]): void {
    const chord = canonicalChord(tokens);
    cancel();
    if (chord) onChange(chord.split("+"));
  }

  function previewFrom(event: KeyboardEvent): string[] {
    const primary = primaryFromKeyboardEvent(event);
    if (primary && !primariesRef.current.includes(primary) && primariesRef.current.length < 4) {
      primariesRef.current = [...primariesRef.current, primary];
    }
    const next = [...modifiersFromKeyboardEvent(event), ...primariesRef.current];
    tokensRef.current = next;
    return next;
  }

  return <button
    type="button"
    className={`chord ${listening ? "listening" : ""}`}
    disabled={disabled}
    aria-label={label}
    aria-pressed={listening}
    onClick={() => {
      if (!listening) start();
    }}
    onKeyDown={(event) => {
      if (!listening || disabled) return;
      event.preventDefault();
      event.stopPropagation();
      if (event.repeat) return;
      if (event.key === "Escape" && primariesRef.current.length === 0 && !event.ctrlKey && !event.altKey && !event.shiftKey) {
        cancel();
        return;
      }
      setPreview(previewFrom(event.nativeEvent));
    }}
    onKeyUp={(event) => {
      if (!listening || disabled) return;
      event.preventDefault();
      event.stopPropagation();
      if (event.ctrlKey || event.altKey || event.shiftKey) return;
      const tokens = tokensRef.current.length > 0 ? tokensRef.current : previewFrom(event.nativeEvent);
      if (canonicalChord(tokens)) commit(tokens);
    }}
    onBlur={() => {
      if (!listening) return;
      if (canonicalChord(tokensRef.current)) commit(tokensRef.current);
      else cancel();
    }}
  >{listening ? preview.join("+") || "按下快捷键…" : canonicalChord(keys) ?? keys.join("+")}</button>;
}
