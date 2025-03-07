# IOCP & Redis Cluster를 활용한 MMO 게임 서버 프로젝트

<br>  

## [소개]

### ㅇGame Server (Redis Cluster) 

  #### 1. 네트워크 최적화
    - Circular Buffer를 활용하여 메모리 사용 최적화
    - 동적 할당 최소화를 위한 설계 적용
    - 대량 데이터 송수신 시 vector 대신 char*형 활용으로 성능 최적화
    - atomic, boost::lockfree_queue, tbb::concurrent_hash_map 등을 활용하여 mutex 사용 최소화
    - Redis 통신 시 try-catch 문을 적용으로 예외 처리 강화
    - MySQL 동기화가 필요한 데이터는 Session Server를 통해 처리하여 부하 분산

  #### 2. 인벤토리 (장비, 소비, 재료)
    - 아이템 획득, 삭제, 슬롯 이동, 장비 강화 시스템 구현
    - 난수 생성 엔진을 활용한 장비 강화 성공 확률 차등 적용

  #### 3. 레이드
    - 레벨 그룹별 레이드 매칭 시스템 구현 
    - 실시간 레이드 전투 시스템 구현 (제한 시간 초과 또는 몬스터 HP 0시 종료) 
    - UDP 기반 IOCP 통신을 활용한 실시간 몬스터 HP 동기화
    - 레이드 랭킹 시스템 구현

  #### 4. 유저 시스템
    - Session Server에서 생성된 JWT 토큰 검증을 통해 접속 요청 유저를 이중 확인하여 보안성 강화
    - 경험치 증가, 레벨업 알고리즘 구현
    - 레벨별 요구 경험치량 설정

### ㅇSession Server (MySQL, Redis Cluster) - User Authentication & Connection Game Server For Syncronization
   - JWT 토큰을 활용한 유저 인증 보안 강화
   - 유저의 게임 시작 요청시, MySQL에서 유저 정보 및 인벤토리 데이터를 Redis Cluster로 load 
   - 유저 로그아웃시, Redis Cluster에 업데이트된 데이터를 MySQL에 동기화 (Batch Update)

### ㅇClient
   - 게임 시작시 Session Server에서 JWT 토큰을 발급 받아 Game Server에 인증 요청
   - 보안 강화를 위해 클라이언트에서 연산 처리를 하지 않도록 설계


<br>  

## [Flow Chart]

- #### User Connect
![Game Server Connect](https://github.com/user-attachments/assets/95b759f4-6a82-4131-9753-174e3fb480ee)

<br>

- #### Raid Start
![Raid Start drawio](https://github.com/user-attachments/assets/c6b74c45-9f12-4ffe-bfbb-c6615d92d8e0)


<br>

- #### Raid End (Time Out)
![Raid Time End](https://github.com/user-attachments/assets/f6fdd216-52fe-40bd-b2b4-600e57a04169)


<br>

- #### Raid End (Mob HP 0)
![Raid mob dead drawio](https://github.com/user-attachments/assets/75b87074-1368-4c1e-9e1f-5430e699937f)


<br>

- #### User Logout or Disconnect
![User Logout drawio](https://github.com/user-attachments/assets/805f11d2-250a-4d60-8874-fad43366fc27)

<br>
<br>

## [Test Output]
<br>

- #### User Connect & Logout & Syncronization
![접속, 접속종료](https://github.com/user-attachments/assets/e9d78268-0fb4-40b1-970f-538dd39c6fc3)

<br>

- #### Raid Start & Raid End & Ranking Update (Mob Hp 0)
![레이드 몹 잡고 랭킹 확인](https://github.com/user-attachments/assets/94eafd7f-08e5-416b-9731-b4465a948b1d)

<br>

- #### Raid Start & Raid End (Time Out)
![타임아웃되면 0점마무리](https://github.com/user-attachments/assets/92dce42d-1204-4fd6-9ccc-69ecd7b07bfb)

<br>

- #### Raid Group Check by Level (Time Out)
![레이드 그룹 체크](https://github.com/user-attachments/assets/f74b7422-cac0-431a-b95a-740e1b5d1dd4)

<br>

- #### Equipment Enhancement
![장비강화](https://github.com/user-attachments/assets/3dc8088e-f5b7-47d5-bef0-d6fe364b13a1)


