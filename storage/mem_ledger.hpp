#ifndef GRUUT_PUBLIC_MERGER_MEM_LEDGER_HPP
#define GRUUT_PUBLIC_MERGER_MEM_LEDGER_HPP

#include "easy_logging.hpp"

#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace gruut {

struct LedgerRecord {
  bool which_scope; // user scope와 contract scope를 구분하기 위한 변수. LedgerType::
  std::string var_name;
  std::string var_val;
  std::string var_type;
  std::string var_owner; // user의 경우 uid, contract의 경우 cid
  timestamp_t up_time;
  block_height_type up_block; // user scope only
  std::string condition;      // user scope only
  std::string var_info;       // contract scope only
  hash_t pid;

  LedgerRecord(std::string var_name_, std::string var_val_, std::string var_type_, std::string var_owner_, timestamp_t up_time_,
               block_height_type up_block_, std::string condition_)
      : var_name(var_name_), var_val(var_val_), var_type(var_type_), var_owner(var_owner_), up_time(up_time_), up_block(up_block_),
        condition(condition_) {
    which_scope = LedgerType::USERSCOPE;
    pid = Sha256::hash(var_name + var_type + var_owner + condition); // 해시될 내용의 처리 방법은 변경될 수 있습니다
  }

  LedgerRecord(std::string var_name_, std::string var_val_, std::string var_type_, std::string var_owner_, timestamp_t up_time_,
               std::string var_info_)
      : var_name(var_name_), var_val(var_val_), var_type(var_type_), var_owner(var_owner_), up_time(up_time_), var_info(var_info_) {
    which_scope = LedgerType::CONTRACTSCOPE;
    pid = Sha256::hash(var_name + var_type + var_owner + var_info); // 해시될 내용의 처리 방법은 변경될 수 있습니다
  }
};

class MemLedger {
private:
  std::list<LedgerRecord> m_ledger;
  std::mutex m_active_mutex;

public:
  MemLedger() {
    el::Loggers::getLogger("MEML");
  }

  bool addUserScope(std::string var_name, std::string var_val, std::string var_type, std::string var_owner, timestamp_t up_time,
                    block_height_type up_block, std::string condition) {
    std::lock_guard<std::mutex> lock(m_active_mutex);
    m_ledger.emplace_back(var_name, var_val, var_type, var_owner, up_time, up_block, condition);

    return true;
  }

  bool addContractScope(std::string var_name, std::string var_val, std::string var_type, std::string var_owner, timestamp_t up_time,
                        std::string var_info) {
    std::lock_guard<std::mutex> lock(m_active_mutex);
    m_ledger.emplace_back(var_name, var_val, var_type, var_owner, up_time, var_info);

    return true;
  }
};
} // namespace gruut

#endif
