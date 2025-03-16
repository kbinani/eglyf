#pragma once

namespace eglyf {

class FeatureVariations {
public:
  struct Condition {
    uint16_t axisIndex;
    F2DOT14 filterRangeMinValue;
    F2DOT14 filterRangeMaxValue;

    static Optional<Condition> Read(InputStream &in) {
      uint16_t format;
      if (!in.u16(&format)) {
        return EGLYF_NULLOPT;
      }
      if (format != 1) {
        return EGLYF_NULLOPT;
      }
      Condition ret;
      if (!in.u16(&ret.axisIndex)) {
        return EGLYF_NULLOPT;
      }
      if (!in.f2dot14(&ret.filterRangeMinValue)) {
        return EGLYF_NULLOPT;
      }
      if (!in.f2dot14(&ret.filterRangeMaxValue)) {
        return EGLYF_NULLOPT;
      }
      return ret;
    }

    Status write(OutputStream &out) const {
      if (!out.u16(1)) {
        return EGLYF_ERROR;
      }
      if (!out.u16(axisIndex)) {
        return EGLYF_ERROR;
      }
      if (!out.f2dot14(filterRangeMinValue)) {
        return EGLYF_ERROR;
      }
      if (out.f2dot14(filterRangeMaxValue)) {
        return Status::Ok();
      } else {
        return EGLYF_ERROR;
      }
    }
  };

  struct ConditionSet {
    static Optional<ConditionSet> Read(InputStream &stream) {
      using namespace std;
      OffsetInputStream in(&stream);
      uint16_t conditionCount;
      if (!in.u16(&conditionCount)) {
        return EGLYF_NULLOPT;
      }
      vector<Offset32> conditionOffsets;
      if (!in.o32a(conditionOffsets, conditionCount)) {
        return EGLYF_NULLOPT;
      }
      ConditionSet ret;
      for (auto offset : conditionOffsets) {
        if (!in.seek(offset)) {
          return EGLYF_NULLOPT;
        }
        if (auto condition = Condition::Read(in); condition) {
          ret.conditions.push_back(*condition);
        } else {
          return EGLYF_NULLOPT_PUSH(condition.status());
        }
      }
      return ret;
    }

    Status write(OutputStream &out) const {
      using namespace std;
      auto writer = make_shared<OffsetWriter>(out);
      if (!out.sizeU16(conditions.size())) {
        return EGLYF_ERROR;
      }
      vector<OffsetWriter::Handle32> conditionOffsets;
      for (size_t i = 0; i < conditions.size(); i++) {
        auto offset = writer->o32();
        if (!offset) {
          return EGLYF_ERROR;
        }
        conditionOffsets.push_back(offset);
      }
      for (size_t i = 0; i < conditions.size(); i++) {
        auto const &condition = conditions[i];
        auto offset = conditionOffsets[i];
        if (auto st = offset->mark(); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
        if (auto st = condition.write(out); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
      }
      return EGLYF_STATUS_PUSH(writer->commit());
    }

    std::vector<Condition> conditions;
  };

  struct FeatureTableSubstitution {
    struct FeatureTableSubstitutionRecord {
      uint16_t featureIndex;
      Offset32 alternateFeatureOffset;

      static Optional<FeatureTableSubstitutionRecord> Read(InputStream &in) {
        FeatureTableSubstitutionRecord ret;
        if (!in.u16(&ret.featureIndex)) {
          return EGLYF_NULLOPT;
        }
        if (!in.o32(&ret.alternateFeatureOffset)) {
          return EGLYF_NULLOPT;
        }
        return ret;
      }
    };

    static Optional<FeatureTableSubstitution> Read(InputStream &in) {
      uint16_t majorVersion;
      uint16_t minorVersion;
      if (!in.u16(&majorVersion)) {
        return EGLYF_NULLOPT;
      }
      if (!in.u16(&minorVersion)) {
        return EGLYF_NULLOPT;
      }
      if (majorVersion != 1 && minorVersion != 0) {
        return EGLYF_NULLOPT;
      }
      uint16_t substitutionCount;
      if (!in.u16(&substitutionCount)) {
        return EGLYF_NULLOPT;
      }
      FeatureTableSubstitution ret;
      for (uint16_t i = 0; i < substitutionCount; i++) {
        if (auto record = FeatureTableSubstitutionRecord::Read(in); record) {
          ret.substitutions.push_back(*record);
        } else {
          return EGLYF_NULLOPT_PUSH(record.status());
        }
      }
      return ret;
    }

    Offset32 featureTableSubstitutionOffset;
    std::vector<FeatureTableSubstitutionRecord> substitutions;
  };

  struct FeatureVariationRecord {
    ConditionSet conditionSet;
    FeatureTableSubstitution featureTableSubstitution;
  };

public:
  static Optional<FeatureVariations> Read(InputStream &stream) {
    using namespace std;
    OffsetInputStream in(&stream);
    uint16_t majorVersion;
    uint16_t minorVersion;
    if (!in.u16(&majorVersion)) {
      return EGLYF_NULLOPT;
    }
    if (!in.u16(&minorVersion)) {
      return EGLYF_NULLOPT;
    }
    if (majorVersion != 1 && minorVersion != 0) {
      return EGLYF_NULLOPT;
    }
    uint32_t featureVariationRecordCount;
    if (!in.u32(&featureVariationRecordCount)) {
      return EGLYF_NULLOPT;
    }
    vector<pair<Offset32, Offset32>> offsets;
    for (uint32_t i = 0; i < featureVariationRecordCount; i++) {
      Offset32 conditionSetOffset;
      Offset32 featureTableSubstitutionOffset;
      if (!in.o32(&conditionSetOffset)) {
        return EGLYF_NULLOPT;
      }
      if (!in.o32(&featureTableSubstitutionOffset)) {
        return EGLYF_NULLOPT;
      }
      offsets.push_back(make_pair(conditionSetOffset, featureTableSubstitutionOffset));
    }
    FeatureVariations ret;
    for (auto [conditionSetOffset, featureTableSubstitutionOffset] : offsets) {
      if (!in.seek(conditionSetOffset)) {
        return EGLYF_NULLOPT;
      }
      FeatureVariationRecord record;

      if (auto conditionSet = ConditionSet::Read(in); conditionSet) {
        record.conditionSet = *conditionSet;
      } else {
        return EGLYF_NULLOPT_PUSH(conditionSet.status());
      }

      if (!in.seek(featureTableSubstitutionOffset)) {
        return EGLYF_NULLOPT;
      }
      if (auto substitution = FeatureTableSubstitution::Read(in); substitution) {
        record.featureTableSubstitution = *substitution;
        record.featureTableSubstitution.featureTableSubstitutionOffset = featureTableSubstitutionOffset;
      } else {
        return EGLYF_NULLOPT_PUSH(substitution.status());
      }
      ret.featureVariationRecords.push_back(record);
    }
    return ret;
  }

public:
  std::vector<FeatureVariationRecord> featureVariationRecords;
};

} // namespace eglyf
