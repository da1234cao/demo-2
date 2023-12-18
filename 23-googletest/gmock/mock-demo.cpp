#include "gmock/gmock.h"
#include <string>

namespace gmock_ {

class Foo {
public:
  // Must be virtual as we'll inherit from Foo.
  virtual ~Foo() {}

  virtual bool SetName(const std::string &name) = 0;
  virtual std::string GetName() const = 0;
  std::string FixName() const;
};

class MockFoo : public Foo {
public:
  MOCK_METHOD(bool, SetName, (const std::string &), (override));
  MOCK_METHOD(std::string, GetName, (), (const, override));
  MOCK_METHOD(std::string, FixName, (), (const));
};

class Area {
public:
  virtual ~Area() {}
  virtual int area() = 0;
};

int GetValue(Area *p) { return p->area() / 2; }

class MockArea : public Area {
public:
  MOCK_METHOD(int, area, (), (override));
};

} // namespace gmock_

TEST(gmock, name) {
  using namespace gmock_;
  using ::testing::AtLeast;
  using ::testing::Return;

  MockFoo foo;
  EXPECT_CALL(foo, SetName("Mike"))
      .Times(AtLeast(2))
      .WillOnce(Return(true))
      .WillOnce(Return(false));
  EXPECT_TRUE(foo.SetName("Mike"));
  EXPECT_FALSE(foo.SetName("Mike"));

  EXPECT_CALL(foo, FixName()).Times(1).WillOnce(Return("Mike"));
  EXPECT_EQ("Mike", foo.FixName());

  EXPECT_CALL(foo, GetName()).Times(AtLeast(1)).WillRepeatedly(Return("Mike"));
  EXPECT_EQ("Mike", foo.GetName());
}

TEST(gmock, area) {
  using namespace gmock_;
  using ::testing::Return;

  MockArea m;
  EXPECT_CALL(m, area()).WillRepeatedly(Return(10));
  EXPECT_EQ(5, GetValue(&m));
}