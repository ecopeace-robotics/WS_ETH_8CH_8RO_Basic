# GitHub 기초 개념 가이드 (초보자용)

> 이 문서는 `git push` 오류를 해결하면서 마주친 개념들을 정리한 것입니다.

---

## 1. Personal Access Token (PAT)

### 무엇인가?

GitHub에 접근하기 위한 **비밀 열쇠**입니다.
예전에는 GitHub 비밀번호로 git push를 했지만, 2021년부터 GitHub는 보안을 위해 비밀번호 대신 **PAT**를 요구합니다.

```
[ 내 컴퓨터 ] --git push--> [ GitHub ]
               ← 인증 필요 ←
               ← PAT 제출 ←
```

### 어디서 만드나?

```
GitHub 프로필 아이콘 → Settings → Developer settings → Personal access tokens → Tokens (classic)
또는 직접: https://github.com/settings/tokens
```

### 토큰 저장 위치

로컬 컴퓨터의 git credential 저장소에 저장됩니다:

```bash
# Linux/macOS: 키체인 또는 아래 경로
~/.git-credentials

# 내용 예시
https://username:ghp_xxxxxxxxxxxx@github.com
```

---

## 2. 스코프(Scope)란?

PAT를 만들 때 **어떤 권한을 줄 것인가**를 설정하는 것입니다.

| 스코프 | 허용 범위 |
|--------|-----------|
| `repo` | 레포지토리 읽기/쓰기 |
| `workflow` | GitHub Actions 워크플로우 파일 수정 |
| `admin:org` | 조직 관리 |
| `read:user` | 사용자 정보 읽기 |

> **원칙:** 필요한 스코프만 선택하는 것이 보안상 좋습니다.
> workflow 스코프 없이 `.github/workflows/` 파일을 push하면 아래 오류가 발생합니다:
> ```
> refusing to allow a Personal Access Token to create or update workflow without `workflow` scope
> ```

---

## 3. GitHub Actions

### 무엇인가?

GitHub에서 제공하는 **자동화 서비스**입니다.
코드를 push하거나 PR을 만들 때, 미리 정해둔 작업을 자동으로 실행합니다.

```
코드 push
   ↓
GitHub Actions 발동
   ↓
자동으로 어떤 작업 실행 (테스트, 빌드, 문서 생성 등)
   ↓
결과 보고
```

### 어디에 설정하나?

레포지토리 루트의 `.github/workflows/` 폴더에 YAML 파일로 정의합니다:

```
내 프로젝트/
├── .github/
│   └── workflows/
│       └── docs.yml       ← 이 파일이 GitHub Actions 설정
├── src/
└── README.md
```

### YAML 파일 예시 (docs.yml)

```yaml
name: 문서 자동 생성

on:
  push:
    branches: [main]     # main 브랜치에 push할 때 실행

jobs:
  build-docs:
    runs-on: ubuntu-latest   # GitHub 서버(우분투)에서 실행

    steps:
      - uses: actions/checkout@v3    # 코드 체크아웃
      - name: Doxygen 설치
        run: sudo apt-get install doxygen
      - name: 문서 생성
        run: doxygen Doxyfile
```

### 실제 활용 예시

| 용도 | 설명 |
|------|------|
| 자동 테스트 | push할 때마다 단위 테스트 자동 실행 |
| 자동 빌드 | 코드를 자동으로 컴파일하고 바이너리 생성 |
| 자동 문서화 | Doxygen으로 API 문서를 자동 생성하고 배포 |
| 자동 배포 | 서버에 최신 코드를 자동으로 올림 |

---

## 4. CI/CD

GitHub Actions의 목적이기도 한 개념입니다.

| 용어 | 뜻 | 예시 |
|------|-----|------|
| **CI** (Continuous Integration) | 지속적 통합 - 코드 변경 시 자동 테스트/빌드 | push할 때마다 컴파일 확인 |
| **CD** (Continuous Deployment) | 지속적 배포 - 자동으로 서버/사이트에 배포 | GitHub Pages에 문서 자동 업로드 |

---

## 5. GitHub Pages

GitHub에서 제공하는 **무료 웹 호스팅** 서비스입니다.
레포지토리의 파일을 웹사이트로 공개할 수 있습니다.

```
레포지토리의 HTML 파일
       ↓
GitHub Pages 활성화
       ↓
https://사용자명.github.io/레포지토리명 에서 접근 가능
```

이 프로젝트에서는 **Doxygen으로 생성된 문서**를 GitHub Pages로 배포하는 것이 목적이었습니다.

---

## 6. 오류 해결 흐름 요약

```
git push 실패
   ↓
오류 메시지 읽기:
"refusing to allow a Personal Access Token to create or update
 workflow ... without `workflow` scope"
   ↓
원인: PAT에 workflow 스코프가 없음
   ↓
해결: GitHub → Settings → PAT → Edit → workflow 체크 → 저장
   ↓
git push 다시 시도
```

---

## 7. 자주 겪는 git push 오류 정리

| 오류 | 원인 | 해결 |
|------|------|------|
| `workflow scope` 오류 | PAT에 workflow 권한 없음 | PAT에 workflow 스코프 추가 |
| `Authentication failed` | 잘못된 토큰 또는 만료 | PAT 재발급 후 credential 업데이트 |
| `rejected - non-fast-forward` | 원격에 내 로컬보다 새 커밋 있음 | `git pull` 후 다시 push |
| `Permission denied` | 레포지토리 접근 권한 없음 | 레포 소유자에게 collaborator 권한 요청 |

---

## 8. 보안 팁

- PAT는 **비밀번호처럼** 취급하세요. 절대 코드에 직접 넣지 마세요.
- PAT는 **필요한 스코프만** 부여하세요.
- PAT에는 **만료일**을 설정하는 것이 좋습니다.
- PAT가 노출되었다면 즉시 GitHub에서 **revoke(취소)** 하세요.

---

*작성일: 2026-03-04*
