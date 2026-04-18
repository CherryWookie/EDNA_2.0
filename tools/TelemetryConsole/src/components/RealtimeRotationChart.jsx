import React, { useState } from "react";
import { LineChart, Line, XAxis, YAxis, Tooltip, CartesianGrid, ResponsiveContainer } from "recharts";

import { sampleOrientationData } from "../sampleData";

function RealtimeRotationChart() {
  const [data, setData] = useState(sampleOrientationData);
  return (
    <ResponsiveContainer>
      <LineChart data={data}>
        <CartesianGrid strokeDasharray="3 3" />
        <XAxis
          dataKey="t"
          tickFormatter={(t) =>
            new Date(t).toLocaleTimeString([], {
              minute: "2-digit",
              second: "2-digit",
            })
          }
        />
        <YAxis domain={[-60, 60]} />
        {/* <Tooltip labelFormatter={(t) => new Date(Number(t)).toLocaleTimeString()} /> */}
        <Line type="monotone" dataKey="yaw" dot={false} isAnimationActive={false} />
        <Line type="monotone" dataKey="pitch" dot={false} isAnimationActive={false} />
        <Line type="monotone" dataKey="roll" dot={false} isAnimationActive={false} />
      </LineChart>
    </ResponsiveContainer>
  );
}

export default RealtimeRotationChart;
