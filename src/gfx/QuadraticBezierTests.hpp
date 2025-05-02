#pragma once

#if EGLYF_ENABLE_TESTS

namespace eglyf::tests {

class QuadraticBezierTests : public juce::UnitTest {
public:
  QuadraticBezierTests() : juce::UnitTest("QuadraticBezierTests") {}

  void runTest() override {
    beginTest("intersects");

    QuadraticBezier<double> q(Vec<double>(135, 756), Vec<double>(165, 731), Vec<double>(192, 707));
    Rect<double> r(163, 725, 345, 902);
    bool hit = q.intersects(r);
    expect(hit);
  }
};

} // namespace eglyf::tests

#endif
