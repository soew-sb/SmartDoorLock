// components/AccessLogView.tsx
import React, { useEffect, useState } from "react";

interface AccessLogEntry {
  timestamp: Date;
  action: "ACCESS_GRANTED" | "ACCESS_DENIED" | "PIN_RESET";
  user?: string;
}

// Mock data generator
const getMockLogs = (): Promise<AccessLogEntry[]> => {
  const mockData: AccessLogEntry[] = [
    {
      timestamp: new Date(Date.now() - 1000 * 60 * 5), // 5 minutes ago
      action: "ACCESS_GRANTED",
    },
    {
      timestamp: new Date(Date.now() - 1000 * 60 * 10), // 10 minutes ago
      action: "ACCESS_DENIED",
    },
    {
      timestamp: new Date(Date.now() - 1000 * 60 * 15), // 15 minutes ago
      action: "PIN_RESET",
    },
    {
      timestamp: new Date(Date.now() - 1000 * 60 * 30), // 30 minutes ago
      action: "ACCESS_GRANTED",
    },
    {
      timestamp: new Date(Date.now() - 1000 * 60 * 45), // 45 minutes ago
      action: "ACCESS_DENIED",
    },
  ];

  return new Promise((resolve) => {
    setTimeout(() => resolve(mockData), 1000); // Simulate network delay
  });
};

const AccessLogView: React.FC = () => {
  const [logs, setLogs] = useState<AccessLogEntry[]>([]);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    getMockLogs()
      .then((data) => {
        setLogs(data);
      })
      .catch((error) => {
        console.error("Error fetching access logs:", error);
      })
      .finally(() => {
        setLoading(false);
      });
  }, []);

  if (loading) {
    return <div className="access-log">Loading access logs...</div>;
  }

  return (
    <div className="access-log">
      <h2>Access History</h2>
      <div className="log-entries">
        {logs.map((log, index) => (
          <div key={index} className="log-entry">
            <span>{new Date(log.timestamp).toLocaleString()}</span>
            <span className={`status ${log.action.toLowerCase()}`}>
              {log.action.replace("_", " ")}
            </span>
            <span>{log.user}</span>
          </div>
        ))}
      </div>
    </div>
  );
};

export { AccessLogView };
