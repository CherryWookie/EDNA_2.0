import React from "react";
import {
  CartesianGrid,
  Line,
  LineChart,
  ResponsiveContainer,
  Tooltip,
  XAxis,
  YAxis,
} from "recharts";

function RealtimeRotationChart({ data }) {
  return (
    <ResponsiveContainer width="100%" height="100%">
      <LineChart data={data} margin={{ top: 8, right: 12, bottom: 4, left: -18 }}>
        <CartesianGrid stroke="#2b3030" strokeDasharray="3 5" />
        <XAxis
          dataKey="t"
          tick={{ fill: "#9aa3a0", fontSize: 12 }}
          tickLine={false}
          axisLine={{ stroke: "#38403e" }}
          minTickGap={28}
          tickFormatter={(t) =>
            new Date(t).toLocaleTimeString([], {
              minute: "2-digit",
              second: "2-digit",
            })
          }
        />
        <YAxis
          domain={[-80, 80]}
          tick={{ fill: "#9aa3a0", fontSize: 12 }}
          tickLine={false}
          axisLine={{ stroke: "#38403e" }}
          unit="deg"
        />
        <Tooltip
          contentStyle={{
            background: "#151918",
            border: "1px solid #35403c",
            borderRadius: 8,
            color: "#f1f5f3",
          }}
          labelFormatter={(t) => new Date(Number(t)).toLocaleTimeString()}
          formatter={(value, name) => [Number(value).toFixed(2), labelFor(name)]}
        />
        <Line
          type="monotone"
          dataKey="roll"
          stroke="#37d399"
          strokeWidth={2}
          dot={false}
          isAnimationActive={false}
        />
        <Line
          type="monotone"
          dataKey="pitch"
          stroke="#f5c84b"
          strokeWidth={2}
          dot={false}
          isAnimationActive={false}
        />
        <Line
          type="monotone"
          dataKey="yaw_rate"
          stroke="#ff6b7a"
          strokeWidth={2}
          dot={false}
          isAnimationActive={false}
        />
      </LineChart>
    </ResponsiveContainer>
  );
}

function labelFor(name) {
  if (name === "yaw_rate") return "Yaw rate";
  return name[0].toUpperCase() + name.slice(1);
}

export default RealtimeRotationChart;
