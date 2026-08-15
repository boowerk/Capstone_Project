---
memoc: true
type: worklog
scope: project-memory
created: 2026-08-13T04:28:58
updated: 2026-08-13T04:28:58
status: active
tags:
  - memoc
  - memoc/worklog
---
# portfolio-contribution-audit-and-production-refresh

actor: dyk66
actor_source: OS user
branch: unknown
status: done
created: 2026-08-13T04:28:58

## Summary

- Git/MEMOC 이력으로 프로덕션 PCG 그래프·서브그래프 15개와 멀티 마을 런타임 생성·동기화가 개인 구현임을 확인하고 외부 PCGEx·아트 및 팀 연동 범위를 분리했다.
- 누락된 Skill/Augment 런타임, 로비·Travel, Zone·NavMesh·안전 배치, Region 표현, 관전·복구, Editor/배포 도구를 개인 포트폴리오 사이트와 PDF에 반영했다.
- 실제 Dedicated Server·3클라이언트 증거와 별도 미촬영 구현을 구분한 채 Vercel 프로덕션에 배포했다.

## Changed Files

- `C:/Users/dyk66/Documents/doyun-game-client-portfolio/src/data/projects.ts`
- `C:/Users/dyk66/Documents/doyun-game-client-portfolio/references/portfolio-content.md`
- `C:/Users/dyk66/Documents/doyun-game-client-portfolio/scripts/build_portfolio_pdf.py`
- `C:/Users/dyk66/Documents/doyun-game-client-portfolio/CAPTURE_GUIDE.md`, `ASSET_MANIFEST.md`

## Verification

- `npm run check`: 오류·경고·힌트 0; 로컬 및 Vercel 정적 빌드 8페이지 성공.
- 10페이지 16:9 PDF를 렌더 검수했고 공개 PDF SHA-256이 로컬 최종본과 일치했다.
- 배포본 1440×900 및 390×844에서 가로 넘침 0, 이미지 6개 정상 로드, 브라우저 오류·경고 0을 확인했다.

## Follow-up

- ACK 재시도는 아직 관찰되지 않았고, 이번에 추가 서술한 보조 시스템은 필요 시 별도 PNG/GIF 실행 증거를 촬영한다.

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/dyk66.md)
