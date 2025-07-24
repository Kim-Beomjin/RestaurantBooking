#pragma once
#include <ctime>
#include "customer.cpp"

using std::time_t;

class Schedule {
public:
	Schedule(tm dateTime, int numberOfPeople, Customer CUSTOMER) :
		dateTime{ dateTime },
		numberOfPeople{ numberOfPeople },
		CUSTOMER{ CUSTOMER } {
	}

	tm getDateTime() {
		return dateTime;
	}

	int getNumberOfPeople() {
		return numberOfPeople;
	}

	Customer getCustomer() {
		return CUSTOMER;
	}

private:
	tm dateTime;
	int numberOfPeople;
	Customer CUSTOMER;
};