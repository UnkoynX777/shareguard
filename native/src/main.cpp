#include "app/Application.hpp"

int wmain(int argc, wchar_t** argv) {
  shareguard::Application application;
  return application.run(argc, argv);
}
