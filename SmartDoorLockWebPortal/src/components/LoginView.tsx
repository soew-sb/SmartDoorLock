// components/LoginView.tsx
import React, { useState } from "react";
import { useNavigate } from "react-router-dom";
import { fakeAuthProvider } from "../auth";

const LoginView: React.FC = () => {
  const [pin, setPin] = useState("");
  const navigate = useNavigate();

  const handleKeyPress = (num: string) => {
    if (pin.length < 4) {
      setPin((prev) => prev + num);
    }
  };

  const handleSubmit = async () => {
    if (pin === "1234") {
      await fakeAuthProvider.signin("User");
      navigate("/access-log");
    } else {
      alert("Incorrect PIN");
      setPin("");
    }
  };

  const handleClear = () => setPin("");

  return (
    <div className="login-container">
      <h2>Enter PIN</h2>
      <div className="pin-display">
        {Array(4)
          .fill("*")
          .map((dot, i) => (
            <span key={i} className={i < pin.length ? "filled" : ""}>
              {i < pin.length ? "●" : "○"}
            </span>
          ))}
      </div>

      <div className="keypad">
        {["1", "2", "3", "4", "5", "6", "7", "8", "9", "C", "0", "E"].map(
          (key) => (
            <button
              key={key}
              onClick={() => {
                if (key === "C") handleClear();
                else if (key === "E") handleSubmit();
                else handleKeyPress(key);
              }}
            >
              {key}
            </button>
          )
        )}
      </div>
    </div>
  );
};

export { LoginView };
