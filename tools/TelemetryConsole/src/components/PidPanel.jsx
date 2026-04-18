import React from "react";
import { Button, NumericInput } from "@blueprintjs/core";

const axes = ["pitch", "roll", "yaw"];
const gains = ["kp", "ki", "kd"];

function PidPanel({ drafts, onDraftsChange, onSend, connected }) {
  return (
    <section className="panel pid-panel">
      <div className="panel-header">
        <div>
          <p className="eyebrow">Control loop</p>
          <h2>PID gains</h2>
        </div>
        <div className="readout-pill">{connected ? "ready" : "connect to send"}</div>
      </div>

      <div className="pid-grid">
        {axes.map((axis) => (
          <article className="pid-axis" key={axis}>
            <header>
              <strong>{axis}</strong>
              <Button
                small
                intent="success"
                icon="send-message"
                disabled={!connected}
                onClick={() => onSend(axis, drafts[axis])}
              >
                Send
              </Button>
            </header>

            {gains.map((gain) => (
              <label className="gain-row" key={gain}>
                <span>{gain.toUpperCase()}</span>
                <NumericInput
                  min={0}
                  stepSize={gain === "ki" ? 0.001 : 0.01}
                  minorStepSize={gain === "ki" ? 0.0001 : 0.001}
                  majorStepSize={gain === "ki" ? 0.01 : 0.1}
                  buttonPosition="none"
                  value={drafts[axis][gain]}
                  onValueChange={(value) =>
                    onDraftsChange({
                      ...drafts,
                      [axis]: {
                        ...drafts[axis],
                        [gain]: Number.isFinite(value) ? value : 0,
                      },
                    })
                  }
                />
              </label>
            ))}
          </article>
        ))}
      </div>
    </section>
  );
}

export default PidPanel;
