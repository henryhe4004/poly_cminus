#pragma once
class GlobalConfig {
  public:
    GlobalConfig() { ir_check = true; }
    bool ir_check;
};
inline GlobalConfig config;
