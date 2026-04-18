import React from "react";
import { Button } from "@blueprintjs/core";
import "./TopBar.css";

function TopBar() {
  return (
    <div>
      <Button intent="primary" icon="link">
        Connect
      </Button>
    </div>
  );
}

export default TopBar;
