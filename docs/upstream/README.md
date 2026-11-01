# Foundation fixes from the purist review

The prepared fixes were committed individually on `main` on 2026-11-01.
The series starts after `54ca05e3785e1ab2771362b150f8050c2a832b9a` and ends at
`eefcfba4c417e94e9a6c5f3af4984d0e14c36505`. The emblem cache fix was already present.
A follow-up [merchant regression update](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/db18fe4b65c59b46ade4908780faadf71341d058)
adapts the existing suite to the corrected restore behavior.

These patch files are historical exports against their recorded base revisions.
Use the commits on `main` for the integrated series. In particular, chat-layout
safe saving follows the literal-name fix. The combined series also reconciles
overlapping test-runner additions and the COM/process-scan startup context.

| Prepared patch | Foundation commit |
| --- | --- |
| [agency-board.patch](agency-board.patch) | [f1af814a4a](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/f1af814a4a469444e0819fb6fb938f5e758c10ca) |
| [attendance-banner-rotation.patch](attendance-banner-rotation.patch) | [38c0d01b9c](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/38c0d01b9cb3dfcb4dd884fc56d291fea01574f6) |
| [call-message-names.patch](call-message-names.patch) | [bf647b1c71](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/bf647b1c71964d3dcf828e9bcb0ebdd2b494df70) |
| [chat-layout-names.patch](chat-layout-names.patch) | [1ddd66c6e3](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/1ddd66c6e33c6f6e3435e45bc9ad9ebf37f6b44d) |
| [chat-layout-safe-save.patch](chat-layout-safe-save.patch) | [df1c2a6f50](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/df1c2a6f50c887f8c1e8bd07235dc39dec88a341) |
| [com-lifecycle.patch](com-lifecycle.patch) | [52258ad4c0](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/52258ad4c03826d0edaa62a11c99180e00cf8d4c) |
| [configured-login-address.patch](configured-login-address.patch) | [785838f7b6](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/785838f7b6098373aa17b68445b845dd9c5aac53) |
| [emblem-cache-name.patch](emblem-cache-name.patch) | [7fe9620feb](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/7fe9620febaabe966c9ddf1356e4f5ab1b9bd144) (already present) |
| [emote-cooldown-rollover.patch](emote-cooldown-rollover.patch) | [51982d6bf4](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/51982d6bf47f782eeca2491445d9b74f571f72a2) |
| [empty-chat-macro.patch](empty-chat-macro.patch) | [783bcd07c0](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/783bcd07c09fa12217504554327ffa4b4f06da13) |
| [empty-gdi-schedule.patch](empty-gdi-schedule.patch) | [bcadf9830e](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/bcadf9830e7f3c611e1295a23d94d4694d2ff4dd) |
| [guild-flag-lazy-loading.patch](guild-flag-lazy-loading.patch) | [af44803888](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/af448038889cf12c7313d0f8880a9ff9c81607a1) |
| [item-restriction-loader.patch](item-restriction-loader.patch) | [faf3195118](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/faf3195118615407dcb0ab5a5474411adcbd89bc) |
| [launch-command-format.patch](launch-command-format.patch) | [fb1896874e](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/fb1896874e305b3009c0fe9ee1681bd8ce41aba8) |
| [map-info-table-bounds.patch](map-info-table-bounds.patch) | [9132b77f69](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/9132b77f696f510fa3b7d6e242a33d8c339ee8f6) |
| [merchant-restore.patch](merchant-restore.patch) | [803f3b0029](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/803f3b002945f84a1f3ec49a6c1759780b2b9f59) |
| [minimap-memo-save-errors.patch](minimap-memo-save-errors.patch) | [25741e1266](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/25741e12660941d2a213be086a937dd7f909ec3c) |
| [navigation-directory.patch](navigation-directory.patch) | [37c824d82f](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/37c824d82fff1569442b612158223367cda7c696) |
| [npc-bgm-playback-state.patch](npc-bgm-playback-state.patch) | [ad5caae375](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/ad5caae3759b85c5a380801bffc86606c073a54c) |
| [pet-portrait-filename.patch](pet-portrait-filename.patch) | [2a29e68653](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/2a29e68653fc1d1360c71b0e462ce113087094b4) |
| [quest-journal-tab-lookup.patch](quest-journal-tab-lookup.patch) | [2a65af5aa0](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/2a65af5aa0457f7e84044a96140b008f35b1e68c) |
| [roulette-prize-order.patch](roulette-prize-order.patch) | [efaaa23b7e](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/efaaa23b7ee578e34ca463aa76ed92ccc78efb7f) |
| [screenshot-save-failures.patch](screenshot-save-failures.patch) | [7e4dfeebeb](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/7e4dfeebeb57ab4c1078aa32b572f4b12242cd3a) |
| [skill-level-registry-validation.patch](skill-level-registry-validation.patch) | [0e2b5e68a7](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/0e2b5e68a737ae52d6e1947674e739e891e1f3cc) |
| [skill-prerequisite-tab-hints.patch](skill-prerequisite-tab-hints.patch) | [d18a160685](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/d18a16068515bf8e8f38fbf3e1fa1451b0ea788b) |
| [startup-registry-reads.patch](startup-registry-reads.patch) | [0956cfe6bd](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/0956cfe6bda69b0e8cc4328abfe25a0cd065ec0a) |
| [unused-startup-process-scan.patch](unused-startup-process-scan.patch) | [b8b622f587](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/b8b622f587d97a55e38cd7a7230755074c080c31) |
| [xml-ignored-field-entities.patch](xml-ignored-field-entities.patch) | [eefcfba4c4](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/eefcfba4c417e94e9a6c5f3af4984d0e14c36505) |

Purist-only removals and local/packet transport replacements remain on `purist`.
The separate server patch exports are documented in [server notes](../server/README.md).
