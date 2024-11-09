// auth.ts
export const fakeAuthProvider = {
  isAuthenticated: false,
  username: null as string | null,
  signin(username: string) {
    return new Promise<void>((resolve) => {
      fakeAuthProvider.isAuthenticated = true;
      fakeAuthProvider.username = username;
      setTimeout(resolve, 100);
    });
  },
  signout() {
    return new Promise<void>((resolve) => {
      fakeAuthProvider.isAuthenticated = false;
      fakeAuthProvider.username = null;
      setTimeout(resolve, 100);
    });
  },
};
