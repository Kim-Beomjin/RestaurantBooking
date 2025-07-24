#pragma once
#include "schedule.cpp"

class MailSender {
public:
	virtual void sendMail(Schedule* schedule) {
		//send mail
	}
};