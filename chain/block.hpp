#ifndef GRUUT_PUBLIC_MERGER_BLOCK_HPP
#define GRUUT_PUBLIC_MERGER_BLOCK_HPP

#include "certificate.hpp"
#include "signature.hpp"
#include "static_merkle_tree.hpp"
#include "transaction.hpp"
#include "types.hpp"

#include "../lib/json.hpp"
#include "../utils/ecdsa.hpp"
#include "../utils/lz4_compressor.hpp"
#include "block_validator.hpp"
#include "easy_logging.hpp"

using namespace std;

namespace gruut {

class Block {
private:
  base58_type m_block_id;
  timestamp_t m_block_time;
  timestamp_t m_block_pub_time;
  world_id_type m_world_id;
  localchain_id_type m_chain_id;
  block_height_type m_block_height;
  base58_type m_block_prev_id;
  base64_type m_block_hash;

  std::vector<txagg_cbor_b64> m_txagg; // Tx의 Json을 CBOR로 처리하고 그 데이터를 b64인코딩한 결과 vector
  std::vector<Transaction> m_transactions; // RDB 블록/트랜잭션 정보에 넣어야하기 때문에 유지. tx_root 계산할 때에도 사용

  base64_type m_aggz; // aggregate signature 에 필요함

  vector<hash_t> m_tx_merkle_tree;
  base64_type m_tx_root;
  base64_type m_us_state_root;
  base64_type m_cs_state_root;
  base64_type m_sg_root;

  std::vector<Signature> m_signers;      // <signer_id, signer_sig> 형태
  std::vector<Certificate> m_user_certs; // <cert_id, cert_content> 형태

  Signature m_block_prod_info;

  string m_block_certificate;

public:
  Block() {
    el::Loggers::getLogger("BLOC");
  };

  bool operator==(Block &other) const {
    return (m_block_height == other.getHeight() && m_block_hash == other.getBlockHash());
  }

  bool initialize(nlohmann::json &msg_block) {

    if (msg_block.empty())
      return false;

    m_block_id = json::get<std::string>(msg_block["block"], "id").value();
    m_block_time = static_cast<gruut::timestamp_t>(stoll(json::get<std::string>(msg_block["block"], "time").value()));
    m_world_id = TypeConverter::base64ToArray<CHAIN_ID_TYPE_SIZE>(json::get<std::string>(msg_block["block"], "world").value());
    m_chain_id = TypeConverter::base64ToArray<CHAIN_ID_TYPE_SIZE>(json::get<std::string>(msg_block["block"], "chain").value());
    m_block_height = stoi(json::get<std::string>(msg_block["block"], "height").value());
    m_block_prev_id = json::get<std::string>(msg_block["block"], "pid").value();
    m_block_hash = json::get<std::string>(msg_block["block"], "hash").value();

    setTxaggs(msg_block["tx"]);
    setTransaction(m_txagg); // txagg에서 트랜잭션 정보를 뽑아낸다

    m_aggz = json::get<std::string>(msg_block, "aggz").value();

    m_tx_merkle_tree = makeStaticMerkleTree(m_txagg);
    m_tx_root = json::get<std::string>(msg_block["state"], "txroot").value();
    m_us_state_root = json::get<std::string>(msg_block["state"], "usroot").value();
    m_cs_state_root = json::get<std::string>(msg_block["state"], "csroot").value();
    m_sg_root = json::get<std::string>(msg_block["state"], "sgroot").value();

    if (!setSigners(msg_block["signer"]))
      return false;
    setUserCerts(msg_block["certificate"]);

    m_block_prod_info.signer_id = json::get<std::string>(msg_block["producer"], "id").value();
    m_block_prod_info.signer_signature = json::get<std::string>(msg_block["producer"], "sig").value();

    m_block_certificate = msg_block["certificate"].dump();

    return true;
  }

  bool setTxaggs(std::vector<txagg_cbor_b64> &txagg) {
    m_txagg = txagg;
    return true;
  }

  bool setTxaggs(nlohmann::json &txs_json) {
    if (!txs_json.is_array()) {
      return false;
    }

    m_txagg.clear();
    for (auto &each_tx_json : txs_json) {
      m_txagg.emplace_back(each_tx_json);
    }
    return true;
  }

  bool setTransaction(std::vector<txagg_cbor_b64> &txagg) {
    m_transactions.clear();
    for (auto &each_txagg : txagg) {
      nlohmann::json each_txs_json;
      each_txs_json = nlohmann::json::from_cbor(TypeConverter::decodeBase<64>(each_txagg));

      Transaction each_tx;
      each_tx.setJson(each_txs_json);
      m_transactions.emplace_back(each_tx);
    }
    return true;
  }

  bool setSupportSignatures(std::vector<Signature> &signers) {
    if (signers.empty())
      return false;
    m_signers = signers;
    return true;
  }

  bool setSigners(nlohmann::json &signers) {
    if (!signers.is_array())
      return false;

    m_signers.clear();
    for (auto &each_signer : signers) {
      Signature tmp;
      tmp.signer_id = json::get<std::string>(each_signer, "id").value();
      tmp.signer_signature = json::get<std::string>(each_signer, "sig").value();
      m_signers.emplace_back(tmp);
    }
    return true;
  }

  bool setUserCerts(nlohmann::json &certificates) {
    m_user_certs.clear();
    for (auto &each_cert : certificates) {
      Certificate tmp;
      tmp.cert_id = json::get<std::string>(each_cert, "id").value();
      tmp.cert_content = json::get<std::string>(each_cert, "cert").value();
      m_user_certs.emplace_back(tmp);
    }
    return true;
  }

  std::vector<txagg_cbor_b64> getTxaggs() {
    std::vector<txagg_cbor_b64> ret_txaggs;
    for (auto &each_tx : m_txagg) {
      ret_txaggs.emplace_back(each_tx);
    }
    return ret_txaggs;
  }

  block_height_type getHeight() {
    return m_block_height;
  }
  size_t getNumTransactions() {
    return m_txagg.size();
  }
  size_t getNumSigners() {
    return m_signers.size();
  }
  timestamp_t getBlockTime() {
    return m_block_time;
  }

  string getBlockIdRaw() {
    return TypeConverter::decodeBase<58>(m_block_id);
  }
  string getBlockId() {
    return m_block_id;
  }

  string getBlockHashRaw() {
    return TypeConverter::decodeBase<64>(m_block_hash);
  }
  string getBlockHash() {
    return m_block_hash;
  }

  string getPrevBlockIdRaw() {
    return TypeConverter::decodeBase<58>(m_block_prev_id);
  }
  string getPrevBlockId() {
    return m_block_prev_id;
  }

  string getTxRoot() {
    return m_tx_root;
  }
};
} // namespace gruut
#endif
