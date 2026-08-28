import { asrHeadline } from "./ui";

export function TranscriptPanel({
  asrState,
  asrText,
  asrReason,
  recState
}: {
  asrState: string | null;
  asrText: string | null;
  asrReason: string | null;
  recState: string | null;
}) {
  const headline = asrHeadline(asrState, asrReason, recState);
  const current = asrText?.trim() ?? "";
  const chip = asrState?.toLowerCase() ?? (recState === "START" || recState === "ACTIVE" ? "start" : recState === "FAIL" ? "fail" : "idle");
  return <section className="transcript" aria-label="转写结果">
    <div className="transcript-head">
      <strong>转写</strong>
      <span className={`asr ${chip}`}>{headline}</span>
    </div>
    <p className={current ? "transcript-text" : "transcript-text empty"}>
      {current || "说完后，转写会出现在这里。"}
    </p>
  </section>;
}
