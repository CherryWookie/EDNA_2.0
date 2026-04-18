import { useEffect, useMemo, useRef, useState } from "react";
import { FocusStyleManager, OverlayToaster, Position } from "@blueprintjs/core";

import "./App.css";
import DroneAttitude from "./components/DroneAttitude";
import MotorPanel from "./components/MotorPanel";
import PidPanel from "./components/PidPanel";
import RealtimeRotationChart from "./components/RealtimeRotationChart";
import TelemetryStats from "./components/TelemetryStats";
import TopBar from "./components/TopBar";
import { defaultPid, emptyTelemetryPacket, useTelemetry } from "./telemetryClient";

FocusStyleManager.onlyShowFocusOnTabs();

const appToaster = OverlayToaster.create({
  maxToasts: 4,
  position: Position.TOP_RIGHT,
});

function App() {
  const telemetry = useTelemetry();
  const lastToastedError = useRef("");
  const [pidDrafts, setPidDrafts] = useState(defaultPid);

  useEffect(() => {
    if (telemetry.latest) {
      setPidDrafts({
        pitch: {
          kp: telemetry.latest.kp_pitch,
          ki: telemetry.latest.ki_pitch,
          kd: telemetry.latest.kd_pitch,
        },
        roll: {
          kp: telemetry.latest.kp_roll,
          ki: telemetry.latest.ki_roll,
          kd: telemetry.latest.kd_roll,
        },
        yaw: {
          kp: telemetry.latest.kp_yaw,
          ki: telemetry.latest.ki_yaw,
          kd: telemetry.latest.kd_yaw,
        },
      });
    }
  }, [telemetry.latest]);

  useEffect(() => {
    if (!telemetry.error || telemetry.error === lastToastedError.current) return;

    lastToastedError.current = telemetry.error;
    appToaster.then((toaster) => {
      toaster.show(
        {
          icon: "error",
          intent: "danger",
          message: telemetry.error,
          timeout: 6500,
        },
        "telemetry-error",
      );
    });
  }, [telemetry.error]);

  const packet = telemetry.latest ?? emptyTelemetryPacket;
  const connectionLabel = telemetry.connected ? "Live UDP" : telemetry.error ? "Link fault" : "Awaiting data";

  const health = useMemo(
    () => ({
      connectionLabel,
      sampleCount: telemetry.history.length,
      ageMs: telemetry.lastSeenAt ? Date.now() - telemetry.lastSeenAt : null,
    }),
    [connectionLabel, telemetry.history.length, telemetry.lastSeenAt],
  );

  return (
    <main className="bp6-dark app-shell">
      <TopBar telemetry={telemetry} health={health} />

      <section className="dashboard-grid" aria-label="Telemetry console">
        <TelemetryStats packet={packet} health={health} />

        <section className="panel attitude-panel">
          <div className="panel-header">
            <div>
              <p className="eyebrow">Airframe</p>
              <h2>Attitude</h2>
            </div>
            <div className="readout-pill">{Math.round(packet.throttle)} us</div>
          </div>
          <DroneAttitude packet={packet} />
        </section>

        <section className="panel chart-panel">
          <div className="panel-header">
            <div>
              <p className="eyebrow">Orientation</p>
              <h2>Roll, pitch, yaw rate</h2>
            </div>
            <div className="legend-row">
              <span className="legend-dot roll" /> Roll
              <span className="legend-dot pitch" /> Pitch
              <span className="legend-dot yaw" /> Yaw rate
            </div>
          </div>
          <RealtimeRotationChart data={telemetry.history.length ? telemetry.history : [packet]} />
        </section>

        {/* <MotorPanel packet={packet} /> */}

        <PidPanel
          drafts={pidDrafts}
          onDraftsChange={setPidDrafts}
          onSend={telemetry.sendPid}
          connected={telemetry.connected}
        />
      </section>
    </main>
  );
}

export default App;
