// components/PinRecoveryView.tsx
import React, { useState, useEffect } from "react";

// Simulate API call
const fetchMasterPin = (): Promise<string> => {
  return new Promise((resolve) => {
    setTimeout(() => {
      resolve("0000"); // Simulated master PIN from API
    }, 1500); // Simulate network delay
  });
};

const PinRecoveryView: React.FC = () => {
  const [loading, setLoading] = useState(true);
  const [masterPin, setMasterPin] = useState<string | null>(null);

  useEffect(() => {
    const getMasterPin = async () => {
      try {
        const pin = await fetchMasterPin();
        setMasterPin(pin);
      } catch (error) {
        console.error("Error fetching master PIN:", error);
      } finally {
        setLoading(false);
      }
    };

    getMasterPin();
  }, []);

  return (
    <div className="pin-recovery">
      <h2>Master PIN Display</h2>
      <div className="pin-display">
        {loading ? (
          <p>Loading master PIN...</p>
        ) : (
          <div>
            <p>Current Master PIN:</p>
            <h3>{masterPin}</h3>
            <small>This PIN can be used to reset the door lock.</small>
          </div>
        )}
      </div>
    </div>
  );
};

export { PinRecoveryView };