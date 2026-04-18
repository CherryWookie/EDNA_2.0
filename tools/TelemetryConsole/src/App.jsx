import { useState } from "react";
import reactLogo from "./assets/react.svg";
import { invoke } from "@tauri-apps/api/core";
import { FocusStyleManager, Colors } from "@blueprintjs/core";

import "./App.css";
import RealtimeRotationChart from "./components/RealtimeRotationChart";
import TopBar from "./components/TopBar";

FocusStyleManager.onlyShowFocusOnTabs();

function App() {
  return (
    <main className="bp6-dark" style={{ backgroundColor: Colors.BLACK }}>
      <TopBar />
      <aside></aside>
      <RealtimeRotationChart />
    </main>
  );
}

export default App;
