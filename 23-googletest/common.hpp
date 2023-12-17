#include <vector>

inline int factorial(int n) {
  std::vector<int> dp(n + 1, 0);
  dp[0] = 1;
  for (int i = 1; i <= n; i++) {
    dp[i] = i * dp[i - 1];
  }
  return dp[n];
}