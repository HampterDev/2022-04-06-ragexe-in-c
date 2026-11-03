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

## Foundation fixes from the size-ranked review, 2026-11-02

These bugs were found while pruning `purist` and were fixed on `main` on
2026-11-02, one commit each with a focused regression test. The series
starts after `db18fe4b65c59b46ade4908780faadf71341d058` and ends at
`1150faabf332e9d5bd99b9df76103db5333aaa2f`; a naming follow-up and the
website-link fix came after it. Fixes whose code still exists
on `purist` were ported with `git cherry-pick -x`; the status-window fix
was rewritten for purist's single-height window. The patch files are
`git format-patch` exports of the `main` commits.

| Patch | Foundation commit | Purist | Change |
| --- | --- | --- | --- |
| [framed-length-below-header.patch](framed-length-below-header.patch) | [ebb1887196](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/ebb18871960a166b57163210b30f78d8c87bc418) | [e731da3cf1](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/e731da3cf16025002914b795d69b10cbb30d0de0) | Reset the connection on a framed packet length below 4 instead of hanging or desynchronizing. |
| [map-entry-reserved-field.patch](map-entry-reserved-field.patch) | [f21e5cb13b](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/f21e5cb13b2f58caaaae4ee7c2d9d6ea9596083e) | [d477f7b91b](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/d477f7b91b186672d62d3c810ae80eb9a98d4941) | Send zeros, not stack bytes, in the unused field of map entry `0x0436`. |
| [name-plate-lookup.patch](name-plate-lookup.patch) | [ad2c6075c8](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/ad2c6075c80a55a58e6b9422d2c726daf8dda66d) | Already correct on purist | Window `0x12d` without a name plate no longer returns the cash shop. |
| [swap-equipment-default-position.patch](swap-equipment-default-position.patch) | [685ebfa786](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/685ebfa78631c403dff69b770da044356e19560e) | Not needed on purist, code removed | The swap-equipment default no longer overwrites the friend-list plate size. |
| [lapine-upgrade-failure-window.patch](lapine-upgrade-failure-window.patch) | [3e9d6ce8fe](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/3e9d6ce8fed8bc3ee7e8e4d55a20150b6dee2c50) | Not needed on purist, code removed | A failed Lapine upgrade releases the upgrade box, not the synthesis box. |
| [status-window-title-bar.patch](status-window-title-bar.patch) | [5da401b8f2](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/5da401b8f2310de64669c67c808e6f54b09955d6) | [fb6d501f97](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/fb6d501f97a00064ebafeefac0a6fe040cc6dea6) (purist version) | A status window saved as a title bar reopens minimized. |
| [option-window-request.patch](option-window-request.patch) | [07135fd6d1](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/07135fd6d18e994eb171b7bc645f52ee575f43bb) | [7bde71e3b3](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/7bde71e3b397f62d492367089af17231c2cbe85b) | A saved `OPTIONWNDINFO.SHOW` no longer requests the nonexistent window `0xd`. |
| [effect-duration-bounds.patch](effect-duration-bounds.patch) | [e26afaf764](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/e26afaf76438c52e774afc32b2d3354af6edc588) | [6095736a2c](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/6095736a2c7411b3313debda12a7a5cde5962e5f) | Effect IDs past the 2,385-row duration table no longer read past its end. |
| [variable-packet-minimum.patch](variable-packet-minimum.patch) | [301c8c62a6](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/301c8c62a62de03d0158857f1be0ca4ce218f3af) | [321284a664](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/321284a664e1a096687d2593fea91fbf787bb53c) | Reject variable-length packets shorter than their table minimum. |
| [esc-list-empty-requests.patch](esc-list-empty-requests.patch) | [1150faabf3](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/1150faabf332e9d5bd99b9df76103db5333aaa2f) | [8cc259bdc5](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/8cc259bdc52e6fed3054b6b06b4e790afa1fedf0) | A window request that leaves no window is not kept in the ESC window list. |
| [sprite-name-tables-and-reserved-field.patch](sprite-name-tables-and-reserved-field.patch) | [cd5635e762](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/cd5635e762b4b5999f6d1cab6d439dfc2ddf3374) | [9b6180b197](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/9b6180b197a546bc1623f6e80c53ec02038d48bc) | Name the sprite-name table builder and the `0x0436` reserved field; no behavior change. |
| [url-link-web-schemes.patch](url-link-web-schemes.patch) | [5238a9f021](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/5238a9f02101792d0995b069ae34361d71b6220d) | [d80fc8010d](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/d80fc8010dd84d2c8c871c8c3fd2e44d11caaf9a) | Text-viewer `<URL>` links open only `http://` and `https://` targets. |

ESC menu command `ESC_ID_1C1` (toggle of window `0xec`) is dead code on both
branches and was left unchanged; see the candidate list in
[purist-review.md](../purist-review.md).

## Foundation fixes from the UTF-8 review, 2026-11-03

These bugs were found during the UTF-8 work and the langtype 20 lock on `purist`
and were fixed on `main` on 2026-11-03, one commit each with a focused
regression test. The series starts after
`5238a9f02101792d0995b069ae34361d71b6220d` and ends at
`d28b5abd09104596bd9ff654209c34a118cd1468`; five follow-up fixes for the bugs it
found, `0fc910c11607f237f7abbb7fd57dfcd8066e7251` to
`6177cea0746fe9e707526edbb18915768d458ec1`, came after it. Fixes whose code still
exists on `purist` were ported with `git cherry-pick -x`; on `purist` the default pet
and homunculus names from job names and the char-server list lines are cut by whole
UTF-8 characters, and the unused login-entry field is removed instead of zeroed. The
patch files are `git format-patch` exports of the `main` commits.

| Patch | Foundation commit | Purist | Change |
| --- | --- | --- | --- |
| [name-text-copies.patch](name-text-copies.patch) | [1b442642e1](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/1b442642e153dcdfc08f1e5cec2171893efef826) | [8016545128](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/80165451283e37faed05a43e6f8962f5af23ed2c) | Copy pet, homunculus, couple, char-server, map and cut-in names, saved styling records and a state line into fixed buffers as text, bounded, instead of as printf formats. |
| [monster-skill-info-values.patch](monster-skill-info-values.patch) | [b071edc1c9](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/b071edc1c93e95c49f65808d9e53abab50fe608c) | [b5ef5609ae](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/b5ef5609aec4ce3396cf8c42d02909b7ea21a5d2) | Read `monsterskillinfo.xml` action and loop values without 8-byte buffers or a format; a missing node is 0. |
| [face-charset-bounds.patch](face-charset-bounds.patch) | [e0db373281](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/e0db373281bd5b97fe6b483309567d70da0f7069) | Already fixed on purist by [103a4735fd](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/103a4735fd70f5611cda14c1774388f13e2e5041) | Faces past the 12-entry `faceCharsets` table no longer read past it and keep the charsets they had. |
| [login-connection-entry-fields.patch](login-connection-entry-fields.patch) | [d28b5abd09](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/d28b5abd09104596bd9ff654209c34a118cd1468) | [d4940e6ba9](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/d4940e6ba9edf53d3963ca4b60a7222c72f64369) (field removed) | Login connection entries no longer copy two fields that nothing set. |
| [server-list-line-cut.patch](server-list-line-cut.patch) | [0fc910c116](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/0fc910c11607f237f7abbb7fd57dfcd8066e7251) | [f0c016a5df](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/f0c016a5df012887e75b75181e14e6c206d729af) (whole UTF-8 characters) | A char-server list line longer than its 64-byte buffer is cut instead of ending the client in `strcpy_s`. |
| [pet-window-names.patch](pet-window-names.patch) | [e07bb1213c](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/e07bb1213c731440b2848c19dd80c759c574baec) | [0af072223c](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/0af072223c036e641ee1342f5c834c8fe1b78c9b) | The pet window and the item collection window get the pet name bounded by its packet field. |
| [short-map-name.patch](short-map-name.patch) | [f41fb0bb5d](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/f41fb0bb5de10e23dfc4e40d637e8c12901f2e5f) | [01f20dc655](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/01f20dc65560c298e083ae6fa9524ed7532dc121) | A server-move map name shorter than its 4-byte extension is kept instead of ending the client. |
| [format-string-arguments.patch](format-string-arguments.patch) | [61939b5d89](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/61939b5d89e6587eab52bcecbd666cb15c7086dc) | [911f3e87ac](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/911f3e87acb96cdbd8f70c284e99289f821b3c6d) | `ZC_FORMATSTRING_MSG` arguments are read only up to the packet length. |
| [talk-line-bounds.patch](talk-line-bounds.patch) | [6177cea074](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/6177cea0746fe9e707526edbb18915768d458ec1) | Not needed on purist, code removed | Pet and monster talk lines are bounded to their buffers and copied as text, not as formats. |

The removal of the dead server types 3 and 13 that came with them is purist-only.
The purist differences, the format-string calls left as they are, the bugs found
and not fixed, and the validation are in
[purist-review.md](../purist-review.md#foundation-fixes-from-the-utf-8-review-2026-11-03).
