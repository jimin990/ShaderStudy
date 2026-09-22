# Direct3D 11
DirectX에 포함된 3D그래픽 API이다.

DirectX는 그래픽만을 담당하는 것이 아니라, 멀티미디어 기술을 초함하는 큰 묶음이고

그 중 그래픽을 담당하는 것이 Direct3D이다.

## Device
```
ComPtr<ID3D11Device> device;
```

Device는 Direct3D의 객체와 GPU 리소스를 생성하는 객체로 그래픽 연산에 필요한 객체들을 생성할 때 사용된다.

앞에 I는 인터페이스를 나타낸다.

```
Vertex Buffer
Index Buffer
Texture
Vertex Shader
Pixel Shader
RenderTargetView
InputLayout
```
와 같은 객체들이 생성된다.

## context

```
ComPtr<ID3D11DeviceContext> context;
```

Context는 앞서 만든 객체들을 실제 렌더링 파이프에 연결, 설정하고 렌더링 명령을 내린다.

# D3D11 렌더링 파이프 라인
D3D11의 렌더링 파이프라인을 핵심 부분은 아래와 같은 순서로 진행된다.
```
IA → VS → Rasterizer → PS → OM
```

IA (Input Assembler): 렌더링 파이프라인의 입력 조립 단계
VS (Vertex Shader): 



## IA (Input Assembler)
렌더링 파이프 라인에서 입력을 조립하는 단계이다.

입력을 조립하는게 추상적이라, 애매할 수 있지만

입력에 필요한 데이터를 지정하고, 입력된 데이터를 어떻게 읽을 지 설정을 한 뒤

어떤 토폴리지로 만들지 지정을 한다.

```
context->IASetVertexBuffers(
    0,
    1,
    &buffer,
    &stride,
    &offset
);
```
buffer를 버텍스버퍼로 지정한다.

이때 버텍스 버퍼를 여러가지로 지정이 될 수 있다.

이때는 buffer을 리스트로 만들어서 값으로 넣으면 된다.

```
context->IASetInputLayout(inputLayout.Get());
```
버퍼 안에 데이터를 어떻게 읽고, 각 값을 어떤 의미로 버텍스 셰이더에 전달할지 정의하는 규칙이다.


# 버텍스 버퍼란?
버텍스 버퍼는 이름 때문에 점의 위치만을 저장하는 버퍼처럼 느껴지지만 실제로는 각 버텍스에 필요한 속성을 저장하는 버퍼로,

다양한 값이 들어 갈 수 있다.

위치는 속성 중 하나로 UV, normal값등 여러 값들이 버텍스 버퍼로 사용될 수 있다.

버퍼는 GPU가 데이터를 읽을 수 있는 메모리 공간이다.

버퍼를 만들기 위해서는, 우선 4가지가 필요하다.

1. 버퍼 설정

BufferDesc로 버퍼의 종류, 용도, 크기 등을 지정할 수 있다.

2. 버퍼에 담을 데이터

버퍼에 담을 데이터가 필요하다.

버텍스 버퍼라면 버텍스가, 인덱스 버퍼라면 인덱스들이 해당된다.

3. 버퍼 초기 데이터

버퍼에 초기에 담을 데이터를 지정 할 수 있다.

앞서 만든 데이터를 여기에 지정한다.

D3D11_SUBRESOURCE_DATA에 버퍼에 담을 초기 데이터를 지정 할 수 있다.

4. 만들어진 버퍼를 담을 포인터

버퍼가 만들어지면 그 버퍼를 담을 포인터가 필요하다.

## 실제 버퍼 만들기

앞선 준비가 되었다면, 실제로 버퍼를 만들 수 있다.

``
device->CreateBuffer(
    &indexBufferDesc,
    &indexData,
    &indexBuffer
);
``
순서대로, 

버퍼 설정, 버퍼 초기 데이터, 만들어진 버퍼를 저장할 포인터이다.

## 버퍼 사용하기
이렇게 버퍼가 만들어지면, Context의 IA를 통해서 버퍼와 레이아웃을 연결해서

실제로 버퍼를 읽을 수 있도록 설정해준다.

여기서 

```
context->IASetVertexBuffers(
    0,
    1,
    &vertexBuffer,
    &stride,
    &offset
);
```

는 버퍼가 버텍스 버퍼라는 것을 지정을 하며, 0번 슬롯을 사용한다고 지정을 한다.

또한 버퍼 안에 데이터 간격과 크기를 지정할 수 있다.

```
context->IASetInputLayout(inputLayout);
```

이후 레이아웃을 지정하여, 이 버퍼를 실제로 GPU가 어떻게 읽는지 규칙을 지정해 준다.

## IA (Input Assembler)
IA 는 정점 입력을 관리하고 조립한다.

정점 입력에는 버텍스 버퍼, 인덱스 버퍼, 인덱스 레이아웃, 토폴리지가 있다.
