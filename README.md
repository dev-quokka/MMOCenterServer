# IOCP & Redis Cluster를 활용한 MMO 게임 서버 프로젝트

<br>  

## [소개]

<br>  

로그인, 친구초대, 친구요청, 친구요청 취소, 친구삭제 시스템을 구현 해 보았습니다.

프로젝트의 주요 코드 설명은 [프로젝트 소개서 [C++ IOCP 프로젝트].pdf](https://github.com/user-attachments/files/18075390/C%2B%2B.IOCP.pdf)


<br>

## [목표]


<br>  

- IOCP를 사용하여 대규모 접속 처리를 해결 해 보며, 비동기 처리와 멀티쓰레드 사용의 숙련도를 높이기 위한 것을 목표로 하였습니다. 

- 많은 내용을 한번에 전송하기 위한 구조체 전송을 가능하게 하는 것을 목표로 하였습니다.

- 구조체는 전부 포인터를 사용하였고, 문자열 사용을 최대한 줄이고 비트단위 자료형 사용, 상황에 맞는 자료구조 사용으로 최적화를 해보는 것을 목표로 하였습니다.


<br>  

## [흐름도]

<br>  

- #### Game Server Connect
![Game Server Connect](https://github.com/user-attachments/assets/1f7b409b-a046-473a-a63a-f2af674ce2ab)
