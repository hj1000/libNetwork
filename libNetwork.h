// 다음 ifdef 블록은 DLL에서 내보내기하는 작업을 쉽게 해 주는 매크로를 만드는 
// 표준 방식입니다. 이 DLL에 들어 있는 파일은 모두 명령줄에 정의된 _EXPORTS 기호로
// 컴파일되며, 동일한 DLL을 사용하는 다른 프로젝트에서는 이 기호를 정의할 수 없습니다.
// 이렇게 하면 소스 파일에 이 파일이 들어 있는 다른 모든 프로젝트에서는 
// LIBNETWORK_API 함수를 DLL에서 가져오는 것으로 보고, 이 DLL은
// 이 DLL은 해당 매크로로 정의된 기호가 내보내지는 것으로 봅니다.
#ifdef LIBNETWORK_EXPORTS
#define LIBNETWORK_API __declspec(dllexport)
#else
#define LIBNETWORK_API __declspec(dllimport)
#endif

// 이 클래스는 libNetwork.dll에서 내보낸 것입니다.
class LIBNETWORK_API ClibNetwork {
public:
	ClibNetwork(void);
	// TODO: 여기에 메서드를 추가합니다.
};

extern LIBNETWORK_API int nlibNetwork;

LIBNETWORK_API int fnlibNetwork(void);
