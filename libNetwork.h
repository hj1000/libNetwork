// ���� ifdef ����� DLL���� ���������ϴ� �۾��� ���� �� �ִ� ��ũ�θ� ����� 
// ǥ�� ����Դϴ�. �� DLL�� ��� �ִ� ������ ��� ����ٿ� ���ǵ� _EXPORTS ��ȣ��
// �����ϵǸ�, ������ DLL�� ����ϴ� �ٸ� ������Ʈ������ �� ��ȣ�� ������ �� �����ϴ�.
// �̷��� �ϸ� �ҽ� ���Ͽ� �� ������ ��� �ִ� �ٸ� ��� ������Ʈ������ 
// LIBNETWORK_API �Լ��� DLL���� �������� ������ ����, �� DLL��
// �� DLL�� �ش� ��ũ�η� ���ǵ� ��ȣ�� ���������� ������ ���ϴ�.
#ifdef LIBNETWORK_EXPORTS
#define LIBNETWORK_API __declspec(dllexport)
#else
#define LIBNETWORK_API __declspec(dllimport)
#endif

// �� Ŭ������ libNetwork.dll���� ������ ���Դϴ�.
class LIBNETWORK_API ClibNetwork {
public:
	ClibNetwork(void);
	// TODO: ���⿡ �޼��带 �߰��մϴ�.
};

extern LIBNETWORK_API int nlibNetwork;

LIBNETWORK_API int fnlibNetwork(void);
