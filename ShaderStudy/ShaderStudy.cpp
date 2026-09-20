#include <iostream>
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <cstring>

/* ComPtr을 사용하기 위한 헤더
*  Comptr은 DirectX 객체를 관리하는 스마트 포인터이다.
*/
#include <wrl/client.h>

/*
* 링커에게 d3d11.lib 라이브러리 연결을 지시한다.
* Direct3D 11 DLL의 함수를 사용하도록 링크를 연결
*/
#pragma comment(lib, "d3d11.lib")

#pragma comment(lib, "d3dcompiler.lib")

using Microsoft::WRL::ComPtr;

struct Vertex
{
    float x, y, z; // 위치
    float r, g, b; // 색
};

/*
* 셰이더 코드
* HSLS 셰이더 코드를 문자열로 보관
*/
const char* shaderSource = R"(
    // 그릴 물체의 크기 변경을 위한 코드
    cbuffer TransformBuffer : register(b0)
    {
        float scale;
        float3 padding;
    };

    struct VSOutput
    {
        float4 position : SV_POSITION;
        float3 color : COLOR;
    };

    VSOutput VSMain(
        float3 position : POSITION,
        float3 color : COLOR)
    {
        VSOutput output;

        position.xy *= scale; // x와 y를 절반으로

        output.position = float4(position, 1.0f);
        output.color = color;

        return output;
    }

    float4 PSMain(VSOutput input) : SV_TARGET
    {
        return float4(input.color, 1.0f);
    }
)";

/*
* 상수 버퍼
* D3D11의 버퍼는 16바이트여야하기 때문에 빈 자리를 배열로 채움
*/
struct TransformData
{
    float scale;
    float padding[3];
};

/*
* LRESULT: 메시지 처리 결과를 반환하는 자료형
* CALLBACK: Windows가 요구하는 함수 호출 규약
* 
*  HWND hwnd: 메시지가 발생한 창의 핸들
*  UINT message: 어떤 메시지인지 나타내는 번호 
*  WPARAM wParam: 메시지에 딸린 추가 정보
*  LPARAM lParam: 메시지에 딸린 추가 정보
*/
LRESULT CALLBACK WindowProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (message) 
    {
    case WM_DESTROY:

        // 창이 파괴되면 메시지 루프에 종료를 알림
        PostQuitMessage(0);
        return 0;
    }

    /*
    * 따로 지정해주지 않은 메시지를 기본 처리 함수에 맡기는 함수
    * 이로서 창 이동, 크기 변경, 닫기 버튼 같은 기본 동작을 하나하나 직접 구현하지 않아도 된다.
    */
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

int WINAPI wWinMain(
    HINSTANCE instance,
    HINSTANCE previousInstance,
    PWSTR commandLine,
    int showCommand)
{
    /*
    * 현재 프로세스의 실행 파일(EXE) 모듈 핸들을 가르킨다.
    * EXE 모듈이란, 현재 실행 중인 EXE의 코드와 데이터라고 생각하면 된다.
    * 프로세스랑 다른 점은 프로세스는 프로그램 전체, 모듈은 그 안에 로드된 구성 요소이다.
    * wWinMain으로 실행하게 되면 인자로 들어오기 때문에 생략한다.
    */
    //HINSTANCE instance = GetModuleHandleW(nullptr);

    /*
    * 창의 기본 설정을 담는 구조체{}는 모두 0으로 초기화 한다는것이다.
    * 여기서 클래스는 c++ 에서 말하는 클래스를 말하는 것이 아니다.
    * 어떤 설정으로 창을 만들지 정해 놓은 틀을 말한다.
    */
    WNDCLASSW wc{};

    /*
    * 창의 메시지를 처리할 함수의 주소를 저장하는 멤버. 즉, 함수 포인터
    * 이 종류의 창에서 메시지가 발생하면 설정한 함수를 호출한다.
    * 이때 함수 호출을 위한 주소를 저장하기 때문에 ()는 붙히지 않는다.
    */
    wc.lpfnWndProc = WindowProc;

    /*
    * 이 창이 속한 모듈을 설정한다.
    * 앞서 만든 모듈을 지정한다.
    */
    wc.hInstance = instance;

    /*
    * 추후 창을 만들때 사용할 이 설정의 이름을 지정한다.
    * 창 제목이랑 다른 개념이다.
    * L 이란 Window의 w버전 함수에서 사용하는 와이드 문자열이라는 표시이다.
    */
    wc.lpszClassName = L"MyWindowClass";

    /*
    * 이 창에서 사용할 마우스 커서를 지정한다.
    * hCursor: 커서 핸들을 저장하는 멤버
    * LoadCursorW: 커서 리소스를 불러오는 함수
    * nullptr을 지정하면 윈도우의 기본 커서를 사용한다.
    * MAKEINTRESOUTCEW: 숫자 리소스 ID를 API에 전달할 수 있는 형태로 바꾼다.
    * 32512: 기본 화살표 커서의 리소스 번호
    */
    wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));

    /*
    * 창 내부의 기본 배경을 지정한다.
    * hbrBackcround: 배경을 칠할 브러시의 핸들
    * GetSysColorBrush: Windows 시스템 색상에 해당하는 브러시를 가져온다.
    * COLOR_WINDOW: 창 내부 배경에 사용하는 시스템 색상
    */
    wc.hbrBackground = GetSysColorBrush(COLOR_WINDOW);

    /*
    * 작성한 설정을 Windows에 등록한다.
    */
    RegisterClassW(&wc);

    // 2. 실제 창 생성
    HWND hwnd = CreateWindowExW(
        0,                      // 추가 스타일
        L"MyWindowClass",       // 등록한 창 클래스 이름
        L"첫 번째 창",          // 제목
        WS_OVERLAPPEDWINDOW,    // 일반적인 데스크톱 창 스타일
        CW_USEDEFAULT,          // 시작 X 위치
        CW_USEDEFAULT,          // 시작 Y 위치
        1000,                    // 창 전체 너비
        1000,                    // 창 전체 높이
        nullptr,                // 부모 창 없음
        nullptr,                // 메뉴 없음
        instance,               // 프로그램 모듈 핸들
        nullptr                 // 추가 전달 데이터 없음
    );

    if (!hwnd)
    {
        // 1은 오류를 나태나는 값
        return 1;
    }

    /*
    * SW_SHOW: 창을 표시하라는 명령
    * 숨기기, 최소화, 최대화 등 여러 명령을 할 수 있다.
    */
    ShowWindow(hwnd, SW_SHOW);

    /*-------------------------------------여기부터 DirectX 설정------------------------------*/

    /*
    * 필요한 자원을 만든다. 예) 버퍼, 셰이더, RTV 생성
    */
    ComPtr<ID3D11Device> device;

    /*
    * 자원을 연결하고 작업을 요청한다.
    */
    ComPtr<ID3D11DeviceContext> context;

    /*
    * 그린 화면을 창에 표기하기 위한 버퍼들을 관리하는 객체
    * 백버퍼를 관리하고, 완성된 화면을 표시하도록 처리하는 객체
    *
    */
    ComPtr<IDXGISwapChain> swapChain;

    /*
    * 백버퍼를 가르킬 스마트 포인터, 백버퍼는 2차원 이미지 자원이다.
    */
    ComPtr<ID3D11Texture2D> backBuffer;

    /*
    * RTV를 가르킬 스마트 포인터
    * RTV란 그 공간을 렌더링 출력 대상으로 사용하기 위한 뷰이다.
    */
    ComPtr<ID3D11RenderTargetView> renderTarget;

    /*
    * SwapChain의 설명서, DESC는 Description을 줄임말
    * 여기서 DXGI란 DirectX Graphics Infrastructure를 의미한다.
    * 그래픽 장치와 화면 출력 관련 기능을 담당.
    * Direct3D는 그리기를 담당하고, DXGI는 그 결과를 창에 표시하는 쪽을 담당한다
    */
    DXGI_SWAP_CHAIN_DESC desc{};

    /*
    * 그림을 담을 버퍼의 크기 지정
    */
    desc.BufferDesc.Width = 800;
    desc.BufferDesc.Height = 600;

    /*
    * 픽셀 하나에 색상을 어떻게 지정할 지 지정
    * R8: 빨강 8비트
    * G8: 초록 8비트
    * B8: 파랑 8비트
    * A8: 알파 8비트
    * UNORM: 저장된 정수값을 사용할때 0~1범위로 해석
    *
    * 8 * 4 = 32비트, 즉 4바이트를 사용한다.
    */
    desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;

    /*
    *
    */
    desc.SampleDesc.Count = 1;

    /*
    * 렌더링 결과를 그려 넣을 버퍼로 사용하겠다고 지정
    */
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

    /*
    * 백 버퍼의 수를 지정
    * 백 버퍼란 그림을 그려두는 공간으로, 현재 화면과 분리해서 미리 다음 화면을 그려둔다.
    * 만약 현재 화면에 다음 화면을 그려버리면 이전화면과 새 화면이 섞여 보일 수 있기 때문이다.
    */
    desc.BufferCount = 1;

    /*
    * 어느 창에 결과를 표기할지 지정
    */
    desc.OutputWindow = hwnd;

    /*
    * 전체화면 독점 모드가 아닌 창모드 사용
    */
    desc.Windowed = TRUE;

    /*
    * 표시 후 버퍼 처리 방식 지정
    * DISCARD의 경우, 이전 내용을 유지하지 않고 다음 화면을 그리도록 한다.
    */
    desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;


    HRESULT result = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &desc,
        swapChain.GetAddressOf(),
        device.GetAddressOf(),
        nullptr,
        context.GetAddressOf()
    );

    if (FAILED(result))
    {
        return -1;
    }

    /*
    * GetBuffer: 이미 swapChain이 만든 버퍼를 가져오는 함수
    */
    result = swapChain->GetBuffer(
        0,
        IID_PPV_ARGS(backBuffer.GetAddressOf())
    );

    if (FAILED(result))
    {
        return -1;
    }

    /*
    * RTV를 만들어달라고 요청하는 함수
    * Get()이란 이미가지고 있는 객체 포인터를 꺼냄
    * GetAddressOf()는 생성 결과 포인터를 써 넣을 자리를 제공
    */
    result = device->CreateRenderTargetView(
        backBuffer.Get(),
        nullptr,
        renderTarget.GetAddressOf()
    );

    ID3D11RenderTargetView* target = renderTarget.Get();

    /*
    * OM이란 Output Merger로 렌더링 결과를 출력 대상에 기록하는 단계이다.
    */
    context->OMSetRenderTargets(
        1,
        &target,
        nullptr
    );
    /*-------------------------------------여기부터 버텍스 설정------------------------------*/

     // 버퍼 크기 지정
    TransformData transformData{};
    transformData.scale = 1.0f;

    D3D11_BUFFER_DESC constantDesc{};
    constantDesc.ByteWidth = sizeof(TransformData);
    constantDesc.Usage = D3D11_USAGE_DEFAULT;
    constantDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    D3D11_SUBRESOURCE_DATA constantInitialData{};
    constantInitialData.pSysMem = &transformData;

    ComPtr<ID3D11Buffer> constantBuffer;

    result = device->CreateBuffer(
        &constantDesc,
        &constantInitialData,
        constantBuffer.GetAddressOf()
    );

    if (FAILED(result))
    {
        return -1;
    }

    ID3D11Buffer* bufferForVS = constantBuffer.Get();

    context->VSSetConstantBuffers(
        0,
        1,
        &bufferForVS
    );

    /*
    * 버텍스, 즉 점 3개가 담긴 배열
    */
    Vertex vertices[] =
    {
        { -0.5f,  0.5f, 0.0f,  1.0f, 0.0f, 0.0f }, // 위: 빨강
        { 0.5f, 0.5f, 0.0f,  0.0f, 1.0f, 0.0f }, // 오른쪽: 초록
        {-0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f },  // 왼쪽: 파랑
        
        { 0.5f,  0.5f, 0.0f,  1.0f, 0.0f, 0.0f }, // 위: 빨강
        { 0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f }, // 오른쪽: 초록
        {-0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f }  // 왼쪽: 파랑
    };

    /*
    * 버퍼를 어떻게 만들지 설정문
    */
    D3D11_BUFFER_DESC bufferDesc{};

    /*
    * 정점 배열 전체를 담을 크기
    * 배열 전체를 담을 공간을 할당
    */
    bufferDesc.ByteWidth = sizeof(vertices);

    /*
    * 생성할 때 데이터를 넣고, 이후에는 변경하지 않음
    */
    bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;

    /*
    * 정점 데이터를 사용하는 버퍼로 사용
    */
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    /*
    * 처음 넣을 데이터 지정
    */
    D3D11_SUBRESOURCE_DATA initialData{};
    initialData.pSysMem = vertices;

    /*
    * 실제 버퍼를 담을 스마트 포인터
    */
    ComPtr<ID3D11Buffer> vertexBuffer;

    /*
    * 실제 버퍼를 생성
    */
    result = device->CreateBuffer(
        &bufferDesc,    // 앞선 설정대로 버퍼를 생성
        &initialData,   // 배열의 데이털 복사해서 넣음
        vertexBuffer.GetAddressOf()
    );

    if (FAILED(result))
    {
        return -1;
    }

    /*
    * 컴파일된 셰이더 코드
    */
    ComPtr<ID3DBlob> vertexShaderCode;
    ComPtr<ID3DBlob> errorMessage;

    result = D3DCompile(
        shaderSource,
        std::strlen(shaderSource),
        nullptr,
        nullptr,
        nullptr,
        "VSMain",
        "vs_5_0",
        0,
        0,
        vertexShaderCode.GetAddressOf(),
        errorMessage.GetAddressOf()
    );

    if (FAILED(result))
    {
        if (errorMessage)
        {
            OutputDebugStringA(
                static_cast<const char*>(
                    errorMessage->GetBufferPointer()
                    )
            );
        }

        return -1;
    }

    /*
    * 버텍스 셰이더 객체
    */
    ComPtr<ID3D11VertexShader> vertexShader;

    result = device->CreateVertexShader(
        vertexShaderCode->GetBufferPointer(),
        vertexShaderCode->GetBufferSize(),
        nullptr,
        vertexShader.GetAddressOf()
    );

    if (FAILED(result))
    {
        return -1;
    }

    // 픽셀 셰이더
    ComPtr<ID3DBlob> pixelShaderCode;

    // 앞에서 사용한 오류 메시지 참조를 비움
    errorMessage.Reset();

    result = D3DCompile(
        shaderSource,
        std::strlen(shaderSource),
        nullptr,
        nullptr,
        nullptr,
        "PSMain",
        "ps_5_0",
        0,
        0,
        pixelShaderCode.GetAddressOf(),
        errorMessage.GetAddressOf()
    );

    if (FAILED(result))
    {
        if (errorMessage)
        {
            OutputDebugStringA(
                static_cast<const char*>(
                    errorMessage->GetBufferPointer()
                    )
            );
        }

        return -1;
    }

    /*
    * 픽셀 셰이더란 삼각형 안에 어떤 색으로 표현을 할지 지정
    */
    ComPtr<ID3D11PixelShader> pixelShader;

    result = device->CreatePixelShader(
        pixelShaderCode->GetBufferPointer(),
        pixelShaderCode->GetBufferSize(),
        nullptr,
        pixelShader.GetAddressOf()
    );

    if (FAILED(result))
    {
        return -1;
    }

    // 정점 처리는 이 버텍스 셰이더를 사용해
    context->VSSetShader(vertexShader.Get(), nullptr, 0);

    // 색상 계산은 이 픽셀 셰이더를 사용해
    context->PSSetShader(pixelShader.Get(), nullptr, 0);

    /*
    * 정점 안에 있는 정보 한 항목을 어떻게 읽을지 설명하는 구조체 변수
    * 위치와 색상, 두 항목이 존재하므로 배열의 크기를 2로 지정
    */
    D3D11_INPUT_ELEMENT_DESC element[2]{};

    // 셰이더의 지정된 이름으로 입력 전달
    element[0].SemanticName = "POSITION";
    element[0].SemanticIndex = 0;

    // 32비트 실수 3개로 읽는다.
    element[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    element[0].InputSlot = 0;

    // 정점의 처음부터 읽는다. 따라서 x,y,z를 읽는다.
    element[0].AlignedByteOffset = 0;
    element[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    element[0].InstanceDataStepRate = 0;


    // 셰이더의 지정된 이름으로 입력 전달
    element[1].SemanticName = "COLOR";
    element[1].SemanticIndex = 0;
    element[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    element[1].InputSlot = 0;

    // 앞선 x,y,z가 12바이트를 차지하기 때문에 그 뒤 부터 읽게 설정한다.
    element[1].AlignedByteOffset = 12;
    element[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    element[1].InstanceDataStepRate = 0;

    /*
    * 입력 레이아웃이란 버텍스 버퍼의 데이터를 어떻게 나눠 읽어서 셰이더에 전달할지 설명서
    */
    ComPtr<ID3D11InputLayout> inputLayout;

    result = device->CreateInputLayout(
        element,
        2,
        vertexShaderCode->GetBufferPointer(),
        vertexShaderCode->GetBufferSize(),
        inputLayout.GetAddressOf()
    );

    if (FAILED(result))
    {
        return -1;
    }

    /*
    * 앞으로 정점데이터를 읽을 때 사용할 레이아웃 지정
    * IA는 Input Assembler
    */
    context->IASetInputLayout(inputLayout.Get());

    /*
    * stride: 한 정점에서 다음 정점까지의 바이트 간격
    * offset: 버퍼의 어디서 부터 읽기 시작할지
    * 현재 위치 + 컬러 = 24바이트, 그러므로 현재 간격은 24바이트이다.
    */
    UINT stride = sizeof(Vertex);
    UINT offset = 0;

    ID3D11Buffer* buffer = vertexBuffer.Get();

    context->IASetVertexBuffers(
        0,
        1,
        &buffer,
        &stride,
        &offset
    );

    // 삼각형으로 연결하도록 지정한다.
    context->IASetPrimitiveTopology(
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
    );

    // 뷰포트 생성
    D3D11_VIEWPORT viewport{};
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = 800.0f;
    viewport.Height = 600.0f;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    //뷰포트 연결
    context->RSSetViewports(1, &viewport);

    /*-------------------------------------여기부터 메시지 루프 설정------------------------------*/

    /*
    * 메시지 정보를 보관할 구조체
    */
    MSG msg{};

    int num{};

    /*
    * 프로그램이 실행되는 동안 메시지를 계속 받아야하기 때문에 반복문을 사용한다.
    */
    while (true)
    {
        wchar_t text[128]{};

        swprintf_s(text, L"%d : 렌더링 시작\n", num);
        OutputDebugStringW(text);

        ++num;

        /*
        * 현재 스레드의 메시지 큐에서 메시지를 가져온다.
        * 메시지는 총 3가지 종류로
        * 양수: 일반 메시지
        * 0: 종료 메시지 WM_QUIT
        * -1: 오류 발생
        * GetMessage는 메시지가 올때 까지 계속 기다리는 상태이다.
        
        BOOL result = GetMessageW(&msg, nullptr, 0, 0);

        if (result == -1) // 오류
        {
            return -1;
        }

        if (result == 0) // WM_QUIT 수신
        {
            break;
        }

        */

        if (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                break;

            /*
            * 메시지값을 실제 문자 메시지 값으로 처리 될 수 있도록 변환한다.
            * 원래의 메시지는 어떤 키를 눌렸다는 처리만 가능하지만, 변환을 통해서
            * 어떤 키를 입력받았는지 알 수 있다.
            * 예를 들어 Shift + a 라면 A키가, 그냥 a 하면 a 키로 입력 받도록 할 수 있는 것이다.
            *
            */
            TranslateMessage(&msg);

            /*
            * 가져온 창 메시지를 해당 창의 처리 함수에 전달
            * 아까 설정한 wc.lpfnWndProc = WindowProc; 이곳에 설정
            */
            DispatchMessageW(&msg);
        }
        else
        {
            const float backgroundColor[4] =
            {
                0.1f, 0.2f, 0.5f, 1.0f
            };

            // 1. 백 버퍼를 배경색으로 채움
            context->ClearRenderTargetView(
                renderTarget.Get(),
                backgroundColor
            );

            // 2. 현재 연결된 버퍼와 셰이더로 삼각형을 그림
            context->Draw(6, 0);

            swapChain->Present(1, 0);
        }
    }

    return static_cast<int>(msg.wParam);
}
