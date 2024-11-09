// App.tsx
import {
  RouterProvider,
  createBrowserRouter,
  redirect,
} from "react-router-dom";
import { fakeAuthProvider } from "./auth";
import { LoginView, AccessLogView, PinRecoveryView } from "./components";
import Layout from "./Layout";
import ProtectedRoute from "./ProtectedRoute";

import "./App.css";

const router = createBrowserRouter([
  {
    path: "/",
    element: <Layout />,
    loader: () => ({ user: fakeAuthProvider.username }),
    children: [
      {
        index: true,
        element: <LoginView />,
      },
      {
        path: "access-log",
        element: (
          <ProtectedRoute>
            <AccessLogView />
          </ProtectedRoute>
        ),
      },
      {
        path: "pin-recovery",
        element: <PinRecoveryView />,
      },
    ],
  },
  {
    path: "/logout",
    action: async () => {
      await fakeAuthProvider.signout();
      return redirect("/");
    },
  },
]);

export default function App() {
  return <RouterProvider router={router} fallbackElement={<p>Loading...</p>} />;
}
