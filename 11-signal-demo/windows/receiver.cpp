#include "common.h"
#include <Windows.h>
#include <iostream>

int main(int argc, char *argv[]) {
  HANDLE events[EVENT_SIZE] = {0};
  for (int i = 0; i < EVENT_SIZE; i++) {
    events[i] = CreateEvent(NULL, TRUE, FALSE, events_name[i]);
  }

  while (1) {
    DWORD event_index =
        WaitForMultipleObjects(EVENT_SIZE, events, FALSE, INFINITE);
    if (event_index == WAIT_OBJECT_0) {
      std::cout << "receive _test_event_one_" << std::endl;
      ResetEvent(events[event_index]);
    } else if (event_index == WAIT_OBJECT_0 + 1) {
      std::cout << "receive _test_event_two_" << std::endl;
      ResetEvent(events[event_index]);
    } else {
      std::cout << "WaitForMultipleObjects return " << event_index << std::endl;
      break;
    }
  }

end:
  // 进程终止时，系统会自动关闭句柄。 事件对象在关闭其最后一个句柄时被销毁
  for (int i = 0; i < EVENT_SIZE; i++) {
    CloseHandle(events[i]);
  }
}