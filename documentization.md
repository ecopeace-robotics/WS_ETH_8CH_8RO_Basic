## 전체 개발 및 문서화 흐름 요약

---

### 도구 구성

| 역할 | 도구 |
|------|------|
| 코드 + 문서 저장 및 공유 | GitHub |
| 프로젝트 전체 설명 | README.md |
| 함수/API 상세 문서 자동 생성 | Doxygen |
| 문서 자동 빌드 | GitHub Actions |
| 문서 웹 호스팅 | GitHub Pages |
| Claude Code 작업 지침 | CLAUDE.md |

---

### 최초 세팅 (프로젝트당 1회)

```
1. GitHub에 저장소 생성
2. 로컬 프로젝트 폴더에 git init 후 연결
3. 프로젝트 유형에 맞는 README 템플릿으로 README.md 작성
4. CLAUDE.md 작성 후 프로젝트 루트에 배치
5. Doxyfile 생성 및 설정
6. GitHub Actions 워크플로우 파일 작성 (.github/workflows/docs.yml)
7. GitHub Pages 활성화
```

---

### 일상적인 개발 흐름

```
코드 작성
    │
    ├── 함수 작성 시 → Doxygen 주석 함께 작성 (@brief @param @return)
    │
    └── git push
            │
            ├── GitHub Actions 자동 실행
            │       └── Doxygen 빌드 → GitHub Pages 배포
            │
            └── 팀원이 URL로 문서 열람
```

---

### README 작성 기준

```
임베디드 프로젝트  →  README_template_embedded.md
ML / AI 프로젝트   →  README_template_ml.md
서버 / SW 프로젝트 →  README_template_server.md
```

---

### Claude Code 활용

```
프로젝트 폴더에서 claude 실행
    │
    └── CLAUDE.md 자동 로드
            │
            ├── "README 작성해줘"        → 템플릿 기준으로 자동 작성
            ├── "Doxygen 주석 추가해줘"  → 누락 함수 우선 작성
            └── "코드 흐름 시각화해줘"   → main 기준 텍스트 트리 생성
```

---

### 현재 준비된 것

- [x] README 템플릿 3종 (embedded / ml / server)
- [x] CLAUDE.md 템플릿
- [ ] 실제 프로젝트에 적용 ← **현재 단계**

다음 단계는 첫 프로젝트에 GitHub 저장소를 만들고 README를 채우는 것입니다. 프로젝트 정보를 알려주시면 바로 도와드릴 수 있습니다.