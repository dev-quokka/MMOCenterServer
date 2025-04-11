#pragma once
class ChannelServer {
public:
	bool InsertUser() {
		if (userCnt.fetch_add(1) + 1 > 30) { // 1�� �����Ѱ� ���� �ο� �ʰ��� -1
			userCnt.fetch_sub(1);
			return false;
		}
		return true;
	}

	void RemoveUser() {
		userCnt.fetch_sub(1);
	}

	uint16_t GetUserCount() const {
		return userCnt.load();
	}

private:
	std::atomic<uint16_t> userCnt = 0;
};