#pragma once

namespace eglyf::cff {

class Dict {
public:
  using Scalar = std::variant<double, int32_t>;
  using Value = std::variant<Scalar, std::vector<Scalar>>;

  Status read(std::string const &data) {
    using namespace std;
    ByteInputStream in(data);
    vector<Scalar> vv;
    while (in.position() < data.size()) {
      uint8_t b0;
      if (!in.u8(&b0)) {
        return EGLYF_ERROR;
      }
      if (b0 == 12 || b0 <= 21) {
        uint8_t key = b0;
        if (b0 == 12) {
          if (!in.u8(&key)) {
            return EGLYF_ERROR;
          }
        }
        if (vv.size() == 1) {
          values[key] = vv[0];
        } else {
          values[key] = vv;
        }
      } else if (32 <= b0 && b0 <= 246) {
        vv.push_back((int32_t)b0 - 139);
      } else if (247 <= b0 && b0 <= 250) {
        uint8_t b1;
        if (!in.u8(&b1)) {
          return EGLYF_ERROR;
        }
        vv.push_back(((int32_t)b0 - 247) * 256 + (int32_t)b1 + 108);
      } else if (251 <= b0 && b0 <= 254) {
        uint8_t b1;
        if (!in.u8(&b1)) {
          return EGLYF_ERROR;
        }
        vv.push_back(-((int32_t)b0 - 251) * 256 - (int32_t)b1 - 108);
      } else if (b0 == 28) {
        uint8_t b1;
        uint8_t b2;
        if (!in.u8(&b1)) {
          return EGLYF_ERROR;
        }
        if (!in.u8(&b2)) {
          return EGLYF_ERROR;
        }
        uint16_t v = (uint16_t(b1) << 8) | uint16_t(b2);
        vv.push_back(*(int16_t *)&v);
      } else if (b0 == 29) {
        uint8_t b1;
        uint8_t b2;
        uint8_t b3;
        uint8_t b4;
        if (!in.u8(&b1)) {
          return EGLYF_ERROR;
        }
        if (!in.u8(&b2)) {
          return EGLYF_ERROR;
        }
        if (!in.u8(&b3)) {
          return EGLYF_ERROR;
        }
        if (!in.u8(&b4)) {
          return EGLYF_ERROR;
        }
        uint32_t v = ((uint32_t(b1) << 24)) | (uint32_t(b2) << 16) | (uint32_t(b3) << 8) | uint32_t(b4);
        vv.push_back(*(int32_t *)&v);
      } else if (b0 == 30) {
        vector<uint8_t> ops;
        while (in.position() < data.size()) {
          uint8_t v;
          if (!in.u8(&v)) {
            return EGLYF_ERROR;
          }
          uint8_t a = v >> 4;
          if (a == 0xf) {
            break;
          }
          ops.push_back(a);
          uint8_t b = 0xf & v;
          if (b == 0xf) {
            break;
          }
          ops.push_back(b);
        }
        uint32_t integer = 0;
        uint32_t decimal = 0;
        int decimalDigits = 0;
        uint32_t exponential = 0;
        int sign = 1;
        int exponentialSign = 1;
        bool hasDecimal = false;
        bool hasExponential = false;
        for (uint8_t op : ops) {
          if (op <= 9) {
            if (hasExponential) {
              exponential = exponential * 10 + op;
            } else if (hasDecimal) {
              decimal = decimal * 10 + op;
              decimalDigits++;
            } else {
              integer = integer * 10 + op;
            }
          } else if (op == 0xA) {
            if (hasDecimal || hasExponential) {
              return EGLYF_ERROR;
            }
            hasDecimal = true;
          } else if (op == 0xb) {
            hasExponential = true;
          } else if (op == 0xc) {
            hasExponential = true;
            exponentialSign = -1;
          } else if (op == 0xd) {
            return EGLYF_ERROR;
          } else if (op == 0xe) {
            if (hasDecimal || hasExponential) {
              return EGLYF_ERROR;
            }
            sign = -1;
          }
        }
        double v = integer * sign;
        if (decimalDigits > 0) {
          v += decimal * pow(10, -decimalDigits);
        }
        if (exponential > 0) {
          v *= pow(10, exponentialSign * exponential);
        }
        vv.push_back(v);
      }
    }
    return Status::Ok();
  }

  std::map<uint8_t, Value> values;
};

} // namespace eglyf::cff