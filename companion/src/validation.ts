import type { DeviceSettings, MappingDraft } from "./types";

const modifiers = ["CTRL", "ALT", "SHIFT"] as const;
const allowedPrimary = new Set([
  ...Array.from({ length: 26 }, (_, index) => String.fromCharCode(65 + index)),
  ...Array.from({ length: 10 }, (_, index) => String(index)),
  ...Array.from({ length: 24 }, (_, index) => `F${index + 1}`),
  "ENTER", "TAB", "ESC", "SPACE", "[", "]"
]);

const namedPrimaries: Record<string, string> = {
  Enter: "ENTER",
  NumpadEnter: "ENTER",
  Tab: "TAB",
  Escape: "ESC",
  Space: "SPACE",
  BracketLeft: "[",
  BracketRight: "]"
};

export interface ParsedChord {
  modifiers: string[];
  primaries: string[];
}

export function parseChordTokens(tokens: readonly string[]): ParsedChord | null {
  const normalized = tokens.map((token) => token.trim().toUpperCase());
  if (normalized.length === 0 || normalized.length > 4 || normalized.some((token) => token.length === 0)) return null;
  if (new Set(normalized).size !== normalized.length) return null;
  const selectedModifiers = modifiers.filter((modifier) => normalized.includes(modifier));
  const primaries = normalized.filter((token) => !modifiers.includes(token as (typeof modifiers)[number]));
  if (primaries.length === 0 || primaries.some((token) => !allowedPrimary.has(token))) return null;
  return { modifiers: selectedModifiers, primaries };
}

export function canonicalChord(tokens: readonly string[]): string | null {
  const parsed = parseChordTokens(tokens);
  return parsed ? [...parsed.modifiers, ...parsed.primaries].join("+") : null;
}

export function chordIdentity(tokens: readonly string[]): string | null {
  const parsed = parseChordTokens(tokens);
  return parsed ? [...parsed.modifiers, ...[...parsed.primaries].sort()].join("+") : null;
}

export function primaryFromKeyboardEvent(event: Pick<KeyboardEvent, "code" | "key">): string | null {
  if (/^Key[A-Z]$/.test(event.code)) return event.code.slice(3);
  if (/^Digit[0-9]$/.test(event.code)) return event.code.slice(5);
  if (/^Numpad[0-9]$/.test(event.code)) return event.code.slice(6);
  const functionKey = event.code.match(/^F([0-9]{1,2})$/);
  if (functionKey) {
    const number = Number(functionKey[1]);
    if (number >= 1 && number <= 24) return `F${number}`;
  }
  if (event.code === "BracketLeft" || event.key === "[" || event.key === "{") return "[";
  if (event.code === "BracketRight" || event.key === "]" || event.key === "}") return "]";
  return namedPrimaries[event.code] ?? namedPrimaries[event.key] ?? null;
}

export function modifiersFromKeyboardEvent(event: Pick<KeyboardEvent, "ctrlKey" | "altKey" | "shiftKey">): string[] {
  return [
    event.ctrlKey ? "CTRL" : null,
    event.altKey ? "ALT" : null,
    event.shiftKey ? "SHIFT" : null
  ].filter((token): token is string => token !== null);
}

export function displayNameError(name: string): string | null {
  const trimmed = name.trim();
  const characters = Array.from(trimmed).length;
  if (characters < 1 || characters > 40) return "名称必须包含 1–40 个字符。";
  if (/\p{Cc}/u.test(trimmed)) return "名称不能包含控制字符。";
  return null;
}

export function mappingErrors(mappings: readonly MappingDraft[]): Map<string, string> {
  const errors = new Map<string, string>();
  const chords = new Map<string, string>();
  for (const mapping of mappings) {
    const nameError = displayNameError(mapping.displayName);
    if (nameError) errors.set(mapping.input, nameError);
    const chord = canonicalChord(mapping.keys);
    const identity = chordIdentity(mapping.keys);
    if (!chord || !identity) {
      errors.set(mapping.input, "请按下至少一个允许的主键，并可同时按 CTRL/ALT/SHIFT，总共最多四个键。");
      continue;
    }
    const duplicate = chords.get(identity);
    if (duplicate) {
      errors.set(mapping.input, `与 ${duplicate} 重复：${chord}。`);
      errors.set(duplicate, `与 ${mapping.input} 重复：${chord}。`);
    } else {
      chords.set(identity, mapping.input);
    }
  }
  return errors;
}

function boundedField(value: string, min: number, max: number, label: string): string | null {
  const characters = Array.from(value).length;
  if (characters < min || characters > max) return `${label}必须包含 ${min}–${max} 个字符。`;
  if (/\p{Cc}/u.test(value)) return `${label}不能包含控制字符。`;
  return null;
}

export function deviceSettingsError(settings: DeviceSettings): string | null {
  return boundedField(settings.ssid, 1, 32, "Wi-Fi 名称")
    ?? boundedField(settings.password, 0, 64, "Wi-Fi 密码");
}

export function ssidLooksFiveG(ssid: string): boolean {
  return /5\s*g(?:hz)?(?![0-9a-z])/i.test(ssid);
}
