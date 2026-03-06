#!/bin/bash
# =============================================================================
# generate_docs.sh
# Doxygen 문서 생성 스크립트
#
# 사용법:
#   ./generate_docs.sh          # 문서 생성
#   ./generate_docs.sh --open   # 문서 생성 후 브라우저 열기
# =============================================================================

set -e  # 오류 발생 시 즉시 중단

# ── 경로 설정 ─────────────────────────────────────────────────────────────────
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VERSION_FILE="$SCRIPT_DIR/VERSION"
DOXYFILE="$SCRIPT_DIR/Doxyfile"
OUTPUT_DIR="$SCRIPT_DIR/docs/doxygen"
LOG_FILE="$SCRIPT_DIR/doxygen_warnings.log"

# ── 인자 파싱 ─────────────────────────────────────────────────────────────────
OPEN_BROWSER=false
if [[ "$1" == "--open" ]]; then
    OPEN_BROWSER=true
fi

# ── 사전 확인 ─────────────────────────────────────────────────────────────────
echo "[1/5] 환경 확인 중..."

if ! command -v doxygen &> /dev/null; then
    echo "오류: doxygen이 설치되어 있지 않습니다."
    exit 1
fi

if ! command -v dot &> /dev/null; then
    echo "경고: Graphviz(dot)가 없습니다. 그래프 없이 문서를 생성합니다."
fi

if [[ ! -f "$VERSION_FILE" ]]; then
    echo "오류: VERSION 파일을 찾을 수 없습니다. ($VERSION_FILE)"
    exit 1
fi

if [[ ! -f "$DOXYFILE" ]]; then
    echo "오류: Doxyfile을 찾을 수 없습니다. ($DOXYFILE)"
    exit 1
fi

# ── VERSION 주입 ──────────────────────────────────────────────────────────────
echo "[2/5] 버전 정보 주입 중..."

VERSION=$(cat "$VERSION_FILE" | tr -d '[:space:]')
echo "  버전: $VERSION"

# Doxyfile의 PROJECT_VERSION 항목을 현재 VERSION으로 교체
sed -i.bak "s/^PROJECT_VERSION\s*=.*/PROJECT_VERSION = \"$VERSION\"/" "$DOXYFILE"
rm -f "$DOXYFILE.bak"

# ── 출력 폴더 초기화 ──────────────────────────────────────────────────────────
echo "[3/5] 출력 폴더 초기화 중..."

if [[ -d "$OUTPUT_DIR" ]]; then
    rm -rf "$OUTPUT_DIR"
fi
mkdir -p "$OUTPUT_DIR"

# ── Doxygen 실행 ──────────────────────────────────────────────────────────────
echo "[4/5] 문서 생성 중..."

# 경고는 로그 파일에 저장, 일반 출력은 터미널에 표시
doxygen "$DOXYFILE" 2> "$LOG_FILE"

# 경고 개수 집계
WARNING_COUNT=$(grep -c "warning:" "$LOG_FILE" 2>/dev/null || echo 0)

if [[ "$WARNING_COUNT" -gt 0 ]]; then
    echo "  경고 $WARNING_COUNT 건 발생 → $LOG_FILE 확인"
else
    echo "  경고 없음"
    rm -f "$LOG_FILE"  # 경고 없으면 로그 파일 삭제
fi

# ── 완료 ──────────────────────────────────────────────────────────────────────
echo "[5/5] 완료"
echo "  출력 경로: $OUTPUT_DIR/html/index.html"

# ── 브라우저 열기 (--open 옵션 시) ───────────────────────────────────────────
if [[ "$OPEN_BROWSER" == true ]]; then
    INDEX="$OUTPUT_DIR/html/index.html"
    if [[ -f "$INDEX" ]]; then
        echo "  브라우저를 엽니다..."
        if command -v xdg-open &> /dev/null; then
            xdg-open "$INDEX"       # Linux
        elif command -v open &> /dev/null; then
            open "$INDEX"           # macOS
        else
            echo "  브라우저를 자동으로 열 수 없습니다. 직접 여세요: $INDEX"
        fi
    fi
fi