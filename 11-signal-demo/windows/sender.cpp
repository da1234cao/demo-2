#include "common.h"
#include <Windows.h>
#include <iostream>

void test_two() {
  HANDLE event_two_handle = CreateEvent(NULL, TRUE, FALSE, events_name[1]);
  SetEvent(event_two_handle);
}

int main(int argc, char *argv[]) {
  HANDLE event_one_handle = CreateEvent(NULL, TRUE, FALSE, events_name[0]);
  SetEvent(event_one_handle);
  if (!ResetEvent(event_one_handle)) {
    std::cout << "event_one_handle reset: " << GetLastError();
  }
  // receiver可能还没来得及重新reset
  if (!SetEvent(event_one_handle)) {
    std::cout << "event_one_handle set: " << GetLastError();
  }

  HANDLE event_two_handle = CreateEvent(NULL, TRUE, FALSE, events_name[1]);
  SetEvent(event_two_handle);
  Sleep(1000);
  ResetEvent(event_two_handle);
  SetEvent(event_two_handle);
  return 0;
}
