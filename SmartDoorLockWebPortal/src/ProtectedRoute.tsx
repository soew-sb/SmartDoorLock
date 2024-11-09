// ProtectedRoute.tsx
import React from "react";
import { Navigate } from "react-router-dom";
import { fakeAuthProvider } from "./auth";

interface ProtectedRouteProps {
  children: React.ReactNode;
}

const ProtectedRoute: React.FC<ProtectedRouteProps> = ({ children }) => {
  if (!fakeAuthProvider.isAuthenticated) {
    return <Navigate to="/" replace />;
  }
  return <>{children}</>;
};

export default ProtectedRoute;