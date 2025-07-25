#include "gmock/gmock.h"
#include "booking_scheduler.cpp"
#include "testable_sms_sender.cpp"
#include "testable_mail_sender.cpp"
#include "testable_booking_scheduler.cpp"

using namespace std;
using namespace testing;

class MockCustomer : public Customer {
public:
	MOCK_METHOD(string, getEmail, (), (override));
};

class BookingItem : public Test {
public:
	tm getTime(int year, int mon, int day, int hour, int min) {
		tm result = { 0, min, hour, day, mon - 1, year - 1900, 0, 0, -1 };
		mktime(&result);
		return result;
	}

	tm plusHour(tm base, int hour) {
		base.tm_hour += hour;
		mktime(&base);
		return base;
	}

	tm NOT_ON_THE_HOUR;
	tm ON_THE_HOUR;
	tm SUNDAY_ON_THE_HOUR;
	tm MONDAY_ON_THE_HOUR;

	MockCustomer CUSTOMER;
	MockCustomer CUSTOMER_WITH_MAIL;
	//Customer CUSTOMER{ "Fake name", "010-1234-5678" };
	//Customer CUSTOMER_WITH_MAIL{ "Fake name", "010-1234-5678", "test@test.com" };
	const int UNDER_CAPACITY = 1;
	const int CAPACITY_PER_HOUR = 3;

	BookingScheduler bookingScheduler{ CAPACITY_PER_HOUR };
	NiceMock<TestableSmsSender> testableSmsSender;
	NiceMock<TestableMailSender> testableMailSender;

protected:
	void SetUp() override {
		NOT_ON_THE_HOUR = getTime(2021, 3, 26, 9, 5);
		ON_THE_HOUR = getTime(2021, 3, 26, 9, 0);
		SUNDAY_ON_THE_HOUR = getTime(2021, 3, 28, 17, 0);
		MONDAY_ON_THE_HOUR = getTime(2024, 6, 3, 17, 0);

		bookingScheduler.setSmsSender(&testableSmsSender);
		bookingScheduler.setMailSender(&testableMailSender);

		EXPECT_CALL((CUSTOMER), getEmail)
			.WillRepeatedly(Return(""));
		EXPECT_CALL((CUSTOMER_WITH_MAIL), getEmail)
			.WillRepeatedly(Return("test@test.com"));

	}
};

TEST_F(BookingItem, 예약은정시에만가능하다정시가아닌경우예약불가) {
	Schedule* schedule = new Schedule{ NOT_ON_THE_HOUR, UNDER_CAPACITY, CUSTOMER };

	EXPECT_THROW({
		bookingScheduler.addSchedule(schedule);
		}, runtime_error);
}

TEST_F(BookingItem, 예약은정시에만가능하다정시인경우예약가능) {
	Schedule* schedule = new Schedule{ ON_THE_HOUR, UNDER_CAPACITY, CUSTOMER };

	bookingScheduler.addSchedule(schedule);

	EXPECT_EQ(true, bookingScheduler.hasSchedule(schedule));
}

TEST_F(BookingItem, 시간대별인원제한이있다같은시간대에Capacity초과할경우예외발생) {
	Schedule* schedule = new Schedule{ ON_THE_HOUR, CAPACITY_PER_HOUR, CUSTOMER };
	bookingScheduler.addSchedule(schedule);

	try {
		Schedule* newSchedule = new Schedule{ ON_THE_HOUR, UNDER_CAPACITY, CUSTOMER };
		bookingScheduler.addSchedule(newSchedule);
		FAIL();
	}
	catch (runtime_error& e) {
		EXPECT_EQ(string{ e.what() },
			string{ "Number of people is over restaurant capacity per hour" });
	}
}

TEST_F(BookingItem, 시간대별인원제한이있다같은시간대가다르면Capacity차있어도스케쥴추가성공) {
	Schedule* schedule = new Schedule{ ON_THE_HOUR, CAPACITY_PER_HOUR, CUSTOMER };
	bookingScheduler.addSchedule(schedule);

	tm differentHour = plusHour(ON_THE_HOUR, 1);
	Schedule* newSchedule = new Schedule{ differentHour, UNDER_CAPACITY, CUSTOMER };
	bookingScheduler.addSchedule(newSchedule);

	EXPECT_EQ(true, bookingScheduler.hasSchedule(schedule));
	EXPECT_EQ(true, bookingScheduler.hasSchedule(newSchedule));
}

TEST_F(BookingItem, 예약완료시SMS는무조건발송) {
	Schedule* schedule = new Schedule{ ON_THE_HOUR, CAPACITY_PER_HOUR, CUSTOMER };

	EXPECT_CALL(testableSmsSender, send(schedule))
		.Times(1);

	bookingScheduler.addSchedule(schedule);
}

TEST_F(BookingItem, 이메일이없는경우에는이메일미발송) {
	Schedule* schedule = new Schedule{ ON_THE_HOUR, CAPACITY_PER_HOUR, CUSTOMER };

	EXPECT_CALL(testableMailSender, sendMail(schedule))
		.Times(0);

	bookingScheduler.addSchedule(schedule);
}

TEST_F(BookingItem, 이메일이있는경우에는이메일발송) {
	Schedule* schedule = new Schedule{ ON_THE_HOUR, CAPACITY_PER_HOUR, CUSTOMER_WITH_MAIL };

	EXPECT_CALL(testableMailSender, sendMail(schedule))
		.Times(1);

	bookingScheduler.addSchedule(schedule);
}

TEST_F(BookingItem, 현재날짜가일요일인경우예약불가예외처리) {
	TestableBookingScheduler mockScheduler{ CAPACITY_PER_HOUR };
	EXPECT_CALL(mockScheduler, getNow)
		.WillRepeatedly(Return(mktime(&SUNDAY_ON_THE_HOUR)));

	BookingScheduler* bookingScheduler = &mockScheduler;

	try {
		Schedule* schedule = new Schedule{ SUNDAY_ON_THE_HOUR, UNDER_CAPACITY, CUSTOMER_WITH_MAIL };
		bookingScheduler->addSchedule(schedule);
		FAIL();
	}
	catch (runtime_error& e) {
		EXPECT_EQ(string{ e.what() },
			string{ "Booking system is not available on sunday" });
	}
}

TEST_F(BookingItem, 현재날짜가일요일이아닌경우예약가능) {
	TestableBookingScheduler mockScheduler{ CAPACITY_PER_HOUR };
	EXPECT_CALL(mockScheduler, getNow)
		.WillRepeatedly(Return(mktime(&MONDAY_ON_THE_HOUR)));

	BookingScheduler* bookingScheduler = &mockScheduler;

	Schedule* schedule = new Schedule{ MONDAY_ON_THE_HOUR, CAPACITY_PER_HOUR, CUSTOMER_WITH_MAIL };
	bookingScheduler->addSchedule(schedule);

	EXPECT_EQ(true, bookingScheduler->hasSchedule(schedule));
}

int main() {
	::testing::InitGoogleMock();
	return RUN_ALL_TESTS();
}