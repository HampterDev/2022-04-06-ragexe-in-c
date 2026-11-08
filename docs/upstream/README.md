# Foundation fixes from the purist review

This index records every fix from `main` and whether `custom` has it. `custom` was called `purist`
until 2026-11-08; the sections before that date use the old name.

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
[custom-review.md](../custom-review.md).

## Foundation fixes from the UTF-8 review, 2026-11-03

These bugs were found during the UTF-8 work and the langtype 20 lock on `purist`
and were fixed on `main` on 2026-11-03, one commit each with a focused
regression test. The series starts after
`5238a9f02101792d0995b069ae34361d71b6220d` and ends at
`d28b5abd09104596bd9ff654209c34a118cd1468`; five follow-up fixes for the bugs it
found, `0fc910c11607f237f7abbb7fd57dfcd8066e7251` to
`6177cea0746fe9e707526edbb18915768d458ec1`, came after it, and five more for the
bugs those found, `9443627d108cad1894173aa5183fb6abd7d8c3c0` to
`eec81169040c52df8336e5d3de97a16ac9ff4879`. The opt-in frame-time overlay,
`53d23a1724793885eb0b6056ac05dcfc5580cdb0`, was approved with them and has its own
row, and a fix, `ab24db87b1f58c8447b01ba62fae3bab9159452c`, came after it. Two more
came from the third-class removal on `purist`: the mercenary job range,
`611062607530a632645a9939d67c8c68193c7a83`, and the constant names,
`278420d369e3e4fb20747bf3257cda4526a2e083`. Two came from the effects-as-data census on
`purist`: the heal-effect types, `b10f009f423ea68c2af14533c6cf87e13f4a5239`, and the
meteor-storm wave frame, `d4921026ecdb808ee9bb20d5b10bd4eac8c2fc87`. One came from the
window-list characterization test on `purist`: the private-airplane forget entry,
`3ac83b2847f151af6deb366560a01c114b080f92`. One came from the window and object-type
tables: the RODEX manager's destroy name, `5572da79175b9354b89d7070ac7d6890af7a4a12`. One came
from the manager lifecycle table: the EZ2STR and map-sky allocation failures, `91ba2d9a01c0ee49cca54a7832e0066ea162764a`,
and the same for the four managers only `main` has, `0b21c238d509de2716fb57edc6ae5cdc15a0cc58`.
Fixes whose code still exists on `purist` were ported with `git cherry-pick -x`; on
`purist` the default pet and homunculus names from job names and the char-server list
lines are cut by whole UTF-8 characters, and the unused login-entry field is removed
instead of zeroed. The patch files are `git format-patch` exports of the `main` commits.

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
| [map-extension-helper.patch](map-extension-helper.patch) | [9443627d10](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/9443627d108cad1894173aa5183fb6abd7d8c3c0) | [199793a7e3](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/199793a7e3538b3b09eb165ad15a96b9a8dbb59e) | The map-move and zone-entry map names shorter than their 4-byte extension are kept too, through one helper. |
| [private-airplane-map-field.patch](private-airplane-map-field.patch) | [1d3102650c](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/1d3102650c7aea72a6b13c3e6f345c5059cb1503) | [89dc33c29f](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/89dc33c29f2d1da69139e0acab46bb426b06cac3) | The private-airplane moves read the map name only from its 16-byte field. |
| [monster-talk-decode.patch](monster-talk-decode.patch) | [378db2f1c8](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/378db2f1c8ab1aa448cb50cd608e0d596e116a1d) | Not needed on purist, code removed | Monster talk keys and lines are decoded in copies, never in a string literal or the table, and the job is bounded. |
| [colour-code-cut.patch](colour-code-cut.patch) | [c7ba72d7ac](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/c7ba72d7ac0b7050daa7a4f423f0aac446afa704) | [8559386948](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/85593869486018cc46280bfc4c1314543bd4ef4b) (whole UTF-8 characters) | A char-server list line cut does not split a `^RRGGBB` colour code. |
| [lead-byte-count.patch](lead-byte-count.patch) | [eec8116904](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/eec81169040c52df8336e5d3de97a16ac9ff4879) | Not needed on purist, which counts with `utf8_next` | The `%s` count of `ZC_FORMATSTRING_MSG` stops at a message that ends in a DBCS lead byte. |
| [overlay-opt-in.patch](overlay-opt-in.patch) | [53d23a1724](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/53d23a1724793885eb0b6056ac05dcfc5580cdb0) | [a25052e7a9](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/a25052e7a9e3ca5db53304a11711fd8ac2a66a2b) | The frame-time overlay appears only with `RAGEXE_OVERLAY` set; Scroll Lock, a screenshot key, no longer shows it. |
| [skin-scan-extensionless.patch](skin-scan-extensionless.patch) | [ab24db87b1](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/ab24db87b1f58c8447b01ba62fae3bab9159452c) | Not needed on purist, code removed | A file without an extension in the `skin` folder is skipped instead of crashing the scan. |
| [mercenary-job-range.patch](mercenary-job-range.patch) | [6110626075](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/611062607530a632645a9939d67c8c68193c7a83) | [a2e90706b6](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/a2e90706b6e02e39bd2b7360d4b56d715961c589) (also stops at 6046) | Job 6017, the first mercenary, counts as a mercenary, so it is shown in the direction rounded from its facing like the others. |
| [rodex-equip-constant-names.patch](rodex-equip-constant-names.patch) | [278420d369](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/278420d369e3e4fb20747bf3257cda4526a2e083) | [0436ca56e9](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/0436ca56e919991f5dfcd1975ef415581eb5d876) (the constants that still exist) | The TaeKwon job IDs and messages in `rodex_compose.c` and the release messages and kinds in `equip_wnd.c` are named for what they are; no behaviour change. |
| [heal-effect-types.patch](heal-effect-types.patch) | [b10f009f42](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/b10f009f423ea68c2af14533c6cf87e13f4a5239) | [8fe0de73b2](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/8fe0de73b224cb1206dcf00daf67c4942ab530ca) | Effects `0x459`, `0x477` and `0x478` play the purple, cyan and yellow heal instead of nothing. |
| [meteor-storm-wave-frame.patch](meteor-storm-wave-frame.patch) | [d4921026ec](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/d4921026ecdb808ee9bb20d5b10bd4eac8c2fc87) | [457d6dd219](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/457d6dd219f8a828423b0fad174cca11d48022fa) | The meteor storm plays its eighth meteor wave at frame 180, as the original, not 170. |
| [airplane-window-forget.patch](airplane-window-forget.patch) | [3ac83b2847](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/3ac83b2847f151af6deb366560a01c114b080f92) | [3df3cbab19](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/3df3cbab19ab10aed83b762b2cabc2f32989a78d) (window-list digest updated) | The private-airplane window's slot is cleared when the window is destroyed, also after a flight ends its modal loop. |
| [rodex-destroy-instance-name.patch](rodex-destroy-instance-name.patch) | [5572da7917](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/5572da79175b9354b89d7070ac7d6890af7a4a12) | [9f11693a89](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/9f11693a89912e75e5844f1280f745545b911d8f) | `set_hwnd_to_game_mon`, the RODEX manager's destroy, is `rodex_destroy_instance`; no behaviour change. |
| [manager-alloc-failure.patch](manager-alloc-failure.patch) | [91ba2d9a01](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/91ba2d9a01c0ee49cca54a7832e0066ea162764a) | [5b707c7249](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/5b707c7249fd9ec51568b824269d10f6eea08889) (the manager table's adapters) | When the EZ2STR or map-sky manager cannot be allocated, its load is skipped and startup goes on; its users take the missing manager as an empty one. |
| [manager-alloc-failure-main-only.patch](manager-alloc-failure-main-only.patch) | [0b21c238d5](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/0b21c238d509de2716fb57edc6ae5cdc15a0cc58) | Not needed on purist, code removed | When the reputation, item-name, item-reform or enchant manager cannot be allocated, its load is skipped and startup goes on; its 29 users take the missing manager as an unloaded one. |

The removal of the dead server types 3 and 13 that came with them is purist-only. The
overlay change is recorded in
[custom-review.md](../custom-review.md#frame-time-overlay-opt-in-2026-11-03).
The purist differences, the format-string calls left as they are, the bugs found
and not fixed, and the validation are in
[custom-review.md](../custom-review.md#foundation-fixes-from-the-utf-8-review-2026-11-03).

## Purist-only fixes after the merge of purist-portable, 2026-11-04

The merge of `purist-portable` (64-bit builds, other platforms) found these. They matter only
where `main` does not build: on Win32 a pointer fits in an `int` and the client text is in the
Windows code page, so `main` has no commit and there are no patch files.

| Purist | On `main` | Change |
| --- | --- | --- |
| [ca17c78818](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/ca17c788185a0e8568cff316c6c08561c26dfd1a) | Same line, harmless (`int` arguments, 32-bit pointers) | Actor message `0x20` hands the chat room to its title window as a whole pointer, not cut to `int`. |
| [b2681a9258](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/b2681a9258c6bda653c578801cd5ea7b650b39b8) | Same line, same results (a 32-bit stack address compares like -1) | The second-latest achievement search starts at -1, not at the address of its output parameter. |
| [3ca656a44f](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/3ca656a44f69a90192c0b44dea349153467de820) | `ShellExecuteA` with CP949 text, as the original | Desktop links open through `ShellExecuteW`, so a UTF-8 URL beyond ASCII is not misread. |
| [aed6618d0d](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/aed6618d0db3ca81facfc46b014c4b473e87da42) | The variables are still used there | Locals the removals left unused are gone; no code change. |

What each does, the sweep, the checks on `main` and the validation are in
[custom-review.md](../custom-review.md#fixes-after-the-merge-of-purist-portable-2026-11-04).

## Foundation fixes ported from `main`, 2026-11-06

`main` had 64 commits that `purist` did not: the 2026-11-02 file-reader, renderer and text
series that came into `main` with its merge of GitHub's `main` (`af2a04f7`), and the
2026-11-05 fixes, ten of them ported from kbirk/fenrir-ragexe. They were picked onto `purist`
in `main`'s order with `git cherry-pick -x`; Kevin Birk's commits keep his authorship. 57 are
on `purist`, 23 of them adapted (each says how under "On purist" in its message), and 7 are left
out. There are no patch files: every commit is on `main`. The adaptations, the bugs found on
the way and the validation are in
[custom-review.md](../custom-review.md#foundation-fixes-ported-from-main-2026-11-06).


| Foundation commit | Purist | Change |
| --- | --- | --- |
| [0d3bad9a92](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/0d3bad9a921a58ac8e49696644766658e645161c) | [b64030b7e9](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/b64030b7e91d97ced192068983db1213fb189391) (adapted) | Give first and second classes JobLevelBase as their maximum job level |
| [60987fd53d](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/60987fd53d947faf1d6f395c92cb3efc29f26085) | [68e9f18759](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/68e9f1875950b354e9c60032b85caee621f7684d) | Keep a sprite whose frame stream does not fill its frame |
| [ac4f50ec8f](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/ac4f50ec8f03ce61e27791cd567b7ba57278ac22) | Already on purist ([b2d7102e98](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/b2d7102e984be48bc2fb78a0358076af719c5c9c)) | Copy pet, homunculus and partner names as text, not as formats |
| [5f5935d450](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/5f5935d4500b52c5c87be0e0b0aa6b00e9164449) | [b715096f76](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/b715096f763b02008665f6ee14223d4f7a36ab30) | Restore the texture arguments of blend modes 6, 7 and 12 |
| [b6fb5c82b5](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/b6fb5c82b5f63490e604c29659b474f5310f5095) | [b8ce42bf1e](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/b8ce42bf1ef415be3f2c65707ed5d70bb1b387ed) (adapted) | Apply the other graphic settings with a live display change |
| [dfc8e98c8a](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/dfc8e98c8a8edabc5bef3171af02e515cb940b78) | [7d405c00d5](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/7d405c00d58a3632c3e45eb7fd8a16cc8e6a0eb6) | Give the shop windows' price limit notice its argument again |
| [e482f8fbb8](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/e482f8fbb8e39f5469de8dc9d8e875244dbe14eb) | [a0981244e0](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/a0981244e0c29869d9724a30d2e6769af868ab13) (adapted) | Wrap bracket groups in descriptions as the original does |
| [362e624d0f](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/362e624d0f92d1b97eced601247f49a0e076714a) | [9f14efb1ac](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/9f14efb1ac4a7b985793be90c66526a811020410) (adapted) | Keep the draw batch on the thread that renders |
| [bcc333b2c4](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/bcc333b2c4e209ec80828c6f70eaeba5590bf8da) | Already on purist (names from `2d82cf71`) | Name the RoDEX recipient check's Taekwon constants for what they are |
| [a5519b29ab](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/a5519b29ab32360270076897025b1099dde2c8a9) | [6d341f6322](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/6d341f632273fdae867fa834d81c82fc46fe5d66) (adapted) | Allow mipmaps on the trilinear filter capability, as the original does |
| [3f58923689](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/3f58923689826a4b1ec9fa9d7b0e3d5c11a89f25) | [053d17bf29](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/053d17bf2984a09016a642088de2be021b3ffdae) | Present the map-loading progress frames without waiting for the refresh |
| [988d58e19c](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/988d58e19c2b066414c1e61eba2e2ca4d54707b8) | [73fa9e89be](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/73fa9e89be2125396624e7c5f8020049da1f25bd) (adapted) | Sleep a millisecond while the game loop draws nothing |
| [6d2a3c2bc8](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/6d2a3c2bc872f8d8674aeca1930c89ad97bfb078) | [5046d2444d](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/5046d2444d0b043270a08e22691513607c64c459) (adapted) | Keep the rotation sin/cos cache per thread |
| [e9d8d0aba0](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/e9d8d0aba035c54cecd0c700ea6c2f74542891a0) | Not needed on purist, code removed (swap-equipment window) | Redraw the Swap button when its cooldown ends |
| [a0d83443e3](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/a0d83443e31dd1afeef8f9229f020150e73745bb) | [446e8e93d7](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/446e8e93d7451157c30ab8a276d40a634e6f03d7) | Animate the navigation preview while its search box has the keyboard |
| [09b0227aa9](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/09b0227aa90d82214e18be2845ad4c7ce357ccd1) | [ea83be1e9b](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/ea83be1e9b8c132fa6d4d38c7af1975c46c19ab6) (adapted) | Follow the screen size after a live resolution change |
| [ff1b3de9e2](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/ff1b3de9e2aa36e5ff7dfaca8f5197702195a22d) | [93781bca6d](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/93781bca6dc20295e70512dc3b04cf4d70832a5c) | Lock the Direct3D 9 backend's surface list |
| [b2eeae4340](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/b2eeae43407087d85a5e0f64693eb592bd86df2c) | [f49b6dac0b](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/f49b6dac0bea82f78075e7508fdc87e29c6b877e) (adapted) | Warn of the restart when a device change cannot be applied live |
| [5457811289](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/5457811289de284eb92a067756dc897388f71138) | [48d5ff172b](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/48d5ff172bae7d54936d629fd2ac70aa37b4011b) | Show the running adapter and an existing mode on the Graphics tab |
| [4fac838e06](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/4fac838e06624fea1346aec1ce6ec6dac5eeb963) | [6a5a23f79f](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/6a5a23f79f1a163fa62324e4dbe57352c2a889f7) | Lay the party plate grid out again for a new screen size |
| [6360d98387](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/6360d9838714c581a48afcca0bc83d0df4d33703) | [2bcb498017](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/2bcb4980176742a9c599d9e69007a47d355700e7) | Load models whose volume-box section is missing or cut short |
| [f459899473](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/f459899473c1bbc8c4e4f56923b9302ba7a300ef) | [34bd97a8ac](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/34bd97a8acf77d120d270802be734efc26f853cf) | Draw 8-bit BMPs that use a palette index past the colours they list |
| [af728d8358](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/af728d83584148f69e5f3a71ace893f93ca2da20) | [30bed89f77](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/30bed89f770346ce4f69aa4ef1646b2e8a228d79) | Hold an STR texture animation on its last frame for an infinite delay |
| [19200b123f](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/19200b123f502b19f07a95462f07fc209fbae4ed) | [68fe45a5fa](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/68fe45a5fa2908d827f3436d368f0baa41baa427) | Keep a GRF entry whose zlib stream does not inflate to its size |
| [33df070cf6](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/33df070cf67ad94615261e8f5d4adc68d69753b6) | [c574d3a77f](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/c574d3a77fa926c4adfb2cabfee7da18290c54a0) (adapted) | Find a broadcast's colon from the start of the text, as the original does |
| [f3a9af9735](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/f3a9af973520b73fc04394960fc4ee27ccb14779) | [63fda36445](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/63fda36445b1c540292aacc9c33dbc4b65cb9abc) | Send a chat line whose item tag is never closed, as the original does |
| [c16ddbf64e](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/c16ddbf64ea3cae697cb3b674c9a8d757fed5053) | [2048de3777](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/2048de37773983baecb414aaea62bae767f119fb) | Stop reading a PNG after its image data, as the original does |
| [dafbcb00f2](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/dafbcb00f2283d207dfd93cb4dafe5c3169ab70d) | [49a0baeccd](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/49a0baeccd6ecced2f844656abae28eb41d016d0) | Measure a '^' in window text the way the original's colour reader does |
| [89a43eec5a](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/89a43eec5a19968c19b08de5f4e2c65dd6257239) | [d43e102d4c](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/d43e102d4c997d5dafdc590f4231d2a17284958a) (adapted) | Decode GIFs with the original's own decoder instead of giflib |
| [95a036f666](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/95a036f66667e7474b823576b9e3129a4570271b) | [a9552f8f67](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/a9552f8f67c19be9e4dfcaa2497988c6421c64d2) (adapted) | Read ItemMoveInfoV5.txt with the original's tab and space tokenizer |
| [854a6d69be](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/854a6d69be2ca8b0195a53dae0a2fce7fff9bd89) | [de95591933](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/de95591933824bf64af0b3d6268c86f4a669d7e7) | Keep an old-format GRF whose record sizes wrap below zero |
| [184e42b811](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/184e42b811156a5e3ae28ea38444116687e65aad) | [d8714b0811](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/d8714b08119faebd86cc279db6103e008b2e77b9) (adapted) | Treat an empty loose file as absent, as the original does |
| [674039728e](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/674039728e2065918fe7c9f60b648d111c920567) | [dcddf8259e](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/dcddf8259e708ac1ad964de3419fb265a893fc0f) (adapted) | Load an RSW with unknown objects, version 0.x or non-finite values |
| [f7f18d1c91](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/f7f18d1c912b546a9ef1b6d9a6020f474f50f165) | [5d0c5f741e](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/5d0c5f741e96997ace768c73dc8046b5097c551c) | Keep a map's water with a negative animation speed or non-finite values |
| [6bd82aaeb8](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/6bd82aaeb8799f8722d63813d7fcd99562027b09) | [e7402ba99a](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/e7402ba99a4fa130d1217d489fe7408ac849197e) | Load a GND with a run-on texture name, no name length or any zoom |
| [54e69baee3](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/54e69baee35d39510add0bace3b390f191c5228b) | [f3c8d7be28](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/f3c8d7be285eb9d490574502902d6b0f44d9757c) | Load a map whose GAT has an unsupported version, with no cells |
| [3a28220ee6](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/3a28220ee63e8210866c6cb15d7ee4d681881f12) | [5831ba3532](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/5831ba35324635f4e158ac206de2e9d6c9dfe379) | Load an RSM2 of an unknown minor version as an empty model |
| [2229dc84d0](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/2229dc84d0dffc9e66d0227fbeac318e44b48801) | [76cf5f8980](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/76cf5f898096288edd1863d852aa306638950491) | Keep an ACT with odd delays, zooms, sizes or a version below 1.0 |
| [2a6f3df1dc](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/2a6f3df1dc65bc75244b040bd1d8e34814bcbd10) | [8b0738f699](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/8b0738f699644d947a25c882a4fb4261670648bf) | Keep an SPR with an empty frame, a short palette or a version below 1.0 |
| [a3452b80c3](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/a3452b80c3f5cd0afbc20c6ed9d876762153b17d) | [8e895acb8f](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/8e895acb8fd97049cc8263a60b60828b5cd4387c) | Hand a PCM WAV with a sloppy RIFF header to Miles, as the original does |
| [e50a066f41](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/e50a066f41cdfd9f4f7bfd9e35b48026f06f516a) | [f37e3f57b2](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/f37e3f57b2fa54793827162ef9be4e88a0eeabe4) | Load a PAL shorter than 1024 bytes with the colours it had |
| [dd6cec07d4](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/dd6cec07d48c9ca77a51064aec155796aed6f3be) | Not needed on purist, code removed (embedded Korean fonts, langtype 20 lock) | Keep the other embedded fonts when one of them fails to load |
| [68d79e9e0a](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/68d79e9e0acb2eef72fb54c204e7e634540ebf8b) | [d00782fb06](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/d00782fb065661ae0b97b7c6f79ff7ee0139cdcf) (adapted) | Show a tool or micc broadcast whose colour is not six hex digits |
| [66fc7c717f](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/66fc7c717fa95e849004740707e99da3d11f3c8c) | [9056ef5313](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/9056ef53133f802dd02b318941cd3a8c22d74d0c) | Read the digits of a ^nItemID^ tag as the original does |
| [52dde89229](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/52dde892296989eab99f379205130eb940137aed) | [18b8aa6d36](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/18b8aa6d361e5387f39e0c5317275805c999531e) (adapted) | Show an item link with more than four cards, ignoring the extra cards |
| [3e6fddfbc0](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/3e6fddfbc027947c41fda20e341f1b34edf04b42) | [c828e76bd8](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/c828e76bd85080f60411fe25b3c7b30d0a4eb457) (adapted) | Finish STR layers and effects by the original's rules |
| [b37b8775fb](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/b37b8775fb9059382df887273d67d4aca33ad2cc) | [726387df28](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/726387df28354594521ecca5a0b44ba8498cb46c) | Load a Granny model without the part the original leaves out |
| [5a63d7b7f3](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/5a63d7b7f39dff1ce350eaad83e2bc8d06e0b391) | [8b8964818f](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/8b8964818fece1618eadb83e0bcd72e00ad43ad3) | Load an IMF with negative counts, no layers or a NaN version |
| [ceba6e9254](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/ceba6e92545b8c098a1be1d11dba59f1c8dd381e) | [e8c1434263](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/e8c14342635610e7e9fea2a89875ee777ce0ada2) | Read BMP headers with only the checks the original makes |
| [a38ec2e1f3](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/a38ec2e1f3638e24a02b9921d0905a2089879e5f) | Not needed: purist's README lists only its deviations | README: what the file readers still refuse, and the reverse STR wrap |
| [e20a417af2](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/e20a417af20623b098fda166ebfff3ba65d8c538) | Not needed: purist's README lists only its deviations | README: models with a short volume-box section still load |
| [996806c15b](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/996806c15ba38623d66db4ed4676d1d884e6e22e) | [ad2ea05617](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/ad2ea056177850cea12c290404e1e14c8b7e29bb) (adapted) | Name ids, messages and functions for what their values are |
| [0e631c8a2c](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/0e631c8a2cc15f78136446c1734fe09a7c088cb4) | [7055aa1aa5](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/7055aa1aa568b8cc828eb28d7e40049b7f6b25dd) (adapted) | Skill use pose: the cases the original's switch reaches through undecoded jump targets (Kevin Birk) |
| [4bb7a8efb3](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/4bb7a8efb327631a079c454abbab7fbc6eec4965) | [3b90e4619f](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/3b90e4619fc2d98969595847e05e47d80b851da1) | Ground skill units: remove a unit at once unless its hits are still playing (Kevin Birk) |
| [8112e2fa06](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/8112e2fa0692a6fa4ce978e10b2174c552f2a336) | [d26550be89](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/d26550be897d949687a7fc20926a723ef09132fd) (adapted) | NPC say dialog: a length field below the header opens an empty page (Kevin Birk) |
| [775e70debf](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/775e70debf0481a115599585a5ef07c15cf0b61f) | [98e171c95e](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/98e171c95e1ac2b923a6b9b8fefe78aa0b2ede76) (adapted) | Talk options: store the name option as an int, as the client does (Kevin Birk) |
| [91c7c294e1](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/91c7c294e1f1b0455a39604d82b32ddb38c63fa5) | [4522d41150](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/4522d411504263ff82157c7c4c0f837538110a96) | Damaged armour line: initialise the String before formatting into it (Kevin Birk) |
| [3057be5371](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/3057be53714ea6587f47cccaa098be4ccc1a71c4) | [476b4b0dfa](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/476b4b0dfa8b62556c2488296c35e8542c5a35c2) | Quest hunting info: look for entry n's objective from slot n on (Kevin Birk) |
| [41778e8ad8](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/41778e8ad813d84de662582c7a023025e1d33708) | Not needed on purist, code removed (macro detector) | Macro detector: acknowledge the picture once every byte is in (Kevin Birk) |
| [0fb6589116](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/0fb6589116a823502fb67a3f879779f2ed5da317) | [6a75d56df1](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/6a75d56df1c841ab182a41ea2c394c45f7c8135b) (adapted) | Add-friend request read past the name (Kevin Birk) |
| [9fc04bb638](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/9fc04bb638cfcc870541b348404721d9ab05aad6) | [220d1837c5](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/220d1837c57e88040c705fd690a9ee14ccdca21c) (adapted) | Read every packet a frame received, even after a map move (Kevin Birk) |
| [0e6e00b6a3](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/0e6e00b6a3af31cc318251057353c73c4e109a52) | [721d4c9316](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/721d4c9316839b171e4209d2ff63bf20ba845d18) | Ground skill effects: as the original's switch, no case 0x21d, one for 0x302 (Kevin Birk) |
| [bf6670583a](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/bf6670583ad3f988402e0b0c61ed56bb6b07cf77) | [037c1658e3](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/037c1658e345478fa62e89f452147e42b9552e27) (adapted) | Cut the Star Gladiator notice's job name to its packet field |
| [34bd3bd2f9](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/34bd3bd2f954c42af315de392a6940558e2fe865) | [8c02cc7c66](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/8c02cc7c6659d766422bf8a9f6a371dc8fa1bf29) | Whip effects: draw the joints' shimmer every frame, as the original does |

## Foundation fixes from the WARP port, 2026-11-06

Found while porting WARP patches to `main` on the `warp-patches` branch, fixed on `main` and
picked onto `purist` with `git cherry-pick -x`.

| Foundation commit | Purist | Change |
| --- | --- | --- |
| [01dcd351aa](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/01dcd351aaf0d80cf373d223676167739a42cc2a) | [9d2ba7b65e](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/9d2ba7b65e1488ca3dae79fde5c386dab87ca5b1) (adapted) | GM name packets: copy the name within its 24-byte field |
| [08e95f6011](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/08e95f6011d98cf9eee4894dbc63f6e416b6b099) | [f980881051](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/f980881051cf3e407e750b7b3f42811a899c745f) (adapted) | Take compress and uncompress from cps.dll, as the original does |
| [d42abc1d26](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/d42abc1d261f5ed89b06d279fb4f6d9b4fe061cb) | [7e9d5d2c38](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/7e9d5d2c388c5fb8752127b2722f7c5740d1de19) (adapted) | Copy nothing from a child window that has no canvas |
| [7bb48c2480](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/7bb48c248070887130db6b010357ec7f6a2f8865) | [3acc6ccdcf](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/3acc6ccdcfa204fcb55056509de3a700af2b8993) (adapted) | Text viewer links: read the parser's link records as they are |
| [c9e9ae7441](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/c9e9ae744119226f340f1a4c962592f4bd56a366) | [8535a6b9a9](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/8535a6b9a9f98fcc56dc5a177135d18c0bb2fdac) (adapted) | Read variable packets shorter than their table minimum, as the original does |
| [bbf0be8ec1](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/bbf0be8ec1d94d292b48d519a8d0248e447277e4) | [f6a797f63a](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/f6a797f63ac74b9fa64222dce59f0a42d18eef69) (adapted) | Item description: a second right-click on the same item closes it |
| [875ce6c0b4](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/875ce6c0b4a9f0250e0236cc7a7c9b789dfc3a5b) | Not needed on purist, code removed (purist starts with defaults already) | Start with the setup defaults when the setup keys are missing |
| [9448917b02](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/9448917b02b986a9f27731526bb3d49c0d94ed06) | [98eb46a8d0](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/98eb46a8d0985dd98ad8119e1a6065441df4b323) | Skip UI quads when a window has no canvas |

## Foundation fixes ported from `main`, 2026-11-08

`main` had moved on to `origin/main` at `b5e3ec71` with commits `custom` had not looked at:
six fixes from 2026-11-01 that predated the earlier ports, the fixes ported from kbirk/fenrir-ragexe,
reverts of fixes that changed the original's own behaviour, the oracle tests, the parity harness,
the function map and documentation. Fixes were picked with `git cherry-pick -x` (each adapted
one says how in its message). The no-canvas fix came in with the effect gallery branch.

| Foundation commit | Custom | Change |
| --- | --- | --- |
| [9448917b02](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/9448917b02b986a9f27731526bb3d49c0d94ed06) | [98eb46a8d0](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/98eb46a8d0985dd98ad8119e1a6065441df4b323) | Skip UI quads when a window has no canvas |
| [430f32c1c9](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/430f32c1c9cf92a907f3f29a84b549942f7e5440) | [5c17aed10a](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/5c17aed10a075e2d8b069dcdd25b66a98e0f40b1) (adapted) | Present the final game frame before a map transition |
| [ac0dc7f555](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/ac0dc7f555fefc3299a690d2deb9c6cceddfb355) | [e2f2685def](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/e2f2685defa2b15c135b344c6bc0dddeb7c6c319) (adapted) | Restore the original whisper title spacing |
| [27f06738cb](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/27f06738cb5a8f7caa38c0ee510c623b8a9344e6) | [d80f44c430](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/d80f44c43018f7b9115ff3329ab2cbcf625b4c9c) | Restore the vending purchase window default geometry |
| [6b430bb85f](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/6b430bb85f751f2ffefad93242d2bad9b4d50d7b) | [246c0bb5ca](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/246c0bb5ca70e16016be2cd71d364f68ad4406db) (adapted) | Select the first character slot when CURSLOT is absent |
| [54ca05e378](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/54ca05e3785e1ab2771362b150f8050c2a832b9a) | [dada202bb6](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/dada202bb64a46a87af87c88427a60a3af7fefa8) (adapted) | Keep vending setup items available after a rejected request |
| [fca5559498](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/fca555949871183a6ddb2a15e51553461de083ae) | [1dec343e36](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/1dec343e360fb061c878ee28784b4c83187c03f4) | Whip effects: draw no joint shimmer, as the original's kind-70 renderer |
| [5e1fe50518](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/5e1fe505189738c17bf646b793cd94633daea718) | [bcdaee4b07](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/bcdaee4b07fafa3786f51dd304253994e52569a9) | Copy a friend's name as its 24 bytes and a NUL |
| [d1cce05e08](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/d1cce05e0882466738c778aa0a4a282605e1ac51) | [0c42c0d3d0](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/0c42c0d3d06a8ce64895a53c02a66fcc96566d92) | Draw nothing for a window that has no canvas |
| [51555aa935](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/51555aa935b242097c17446f520374eb27d241cd) | [d849919425](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/d84991942563b99df6c5ebf4b4fb1a838e11dd11) | Item count window: the item's name cut to the label with "..." when shown |
| [321658bb8e](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/321658bb8ec2a3b5b49109ebeff4e909c78b2d70) | [df6e0e8664](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/df6e0e8664f9364e174920571d83e52eae57c69e) | Ring ripple: restart a ring at the client's radius, 0x4008f5c2 |
| [1450ad0d53](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/1450ad0d53bbe3c6c13c03f9b5a7f7b9fbb40b83) | [129e2b5889](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/129e2b588935702bef2d0dbc37625947e677ca68) | World view: a style value the Lua function does not return reads as 0 |
| [54ed1f209d](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/54ed1f209d632ff1e45908524f408241b469fa47) | [6354bfbce3](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/6354bfbce3df6659fec3c71262312f22fa7e4142) | Skill shout: a long skill name runs into the line buffer, as in the original's frame |
| [ff1a1f05e7](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/ff1a1f05e778afa795a85bd29be9120d125965d9) | [2377a39fa9](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/2377a39fa9aed02913d8fd8018f2e755200d33b2) (adapted) | Quest journal: look for a requested quest in three tabs, as the original |
| [874444c452](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/874444c4520ce4b8a460214a20eeb7b159407d9a) | [f8ae426e23](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/f8ae426e23b58f3a8c83105cd5df1441df5e9ba6) (adapted, with 3aa2e000) | Revert "Restore map music only after NPC music finishes" |
| [3aa2e0003e](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/3aa2e0003eb0ecadaa8c99a8635d50b9d5752dca) | [f8ae426e23](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/f8ae426e23b58f3a8c83105cd5df1441df5e9ba6) (adapted, with 874444c4) | Name the music stream's done test sound_stream_done |
| [829aae0ef0](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/829aae0ef0dba56bdbc9010e404a6ce74d9cc603) | [26dc533107](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/26dc533107fd444f37ad9ae97dc266566c7bf49f) | tools/funcmap: the original-address to lifted-function map |
| [9150627e8e](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/9150627e8e760abdfe32e3ccb43686b47b57b85e) | [c18b6d1f2d](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/c18b6d1f2dd2636f9caa4d98fb60f561e9bb7129) | tools/funcmap: the switch sweeps, every jump table's values against the lifted switches |
| [0aa00b340d](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/0aa00b340dec3923443442f9995f6433afe81045) | [2ec8b4bf2e](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/2ec8b4bf2ee281da52219304925e9e3caa2a2278) (adapted) | tools, tests: the tool docs speak of the assembly listing and the annotated C, not of where they came from |
| [63b3843a71](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/63b3843a71011603fd6445cda7f138d1e69f8000) | [143f5fbb11](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/143f5fbb11e6e8c23a933702d811440b978dbf5a) | tools/funcmap: check the map against the dump's function starts and the oracle's pairs |
| [3a9e094a2f](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/3a9e094a2f59bbc3d7dc0f86921f7855da4a2042) | [ca55dfd2dd](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/ca55dfd2dde5192437329e49ab776fdfaab6c8f4) | Turn off Claude Code commit and PR attribution |
| [78d599c0bf](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/78d599c0bf203c67db1a97d29cc2f399831de9a4) | [536f3bad6a](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/536f3bad6a4a9b5633eb3196ac5f96bfdb33bd46) (adapted) | build.bat: name the exe ragexe.exe |
| [581d3505db](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/581d3505dbdae380c0a6b213e7d8ff647f743960) | Already correct on custom: its window table has no rows for 0x166 and 0x167 | Make no window for ids 0x166 and 0x167 |
| [41a072d62a](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/41a072d62ab267affd902f54459c524c52a27a95) | Not needed on custom, code removed (replay control) | Replay control: give the location its own static text, past a hidden one |
| [9631d4fab9](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/9631d4fab9392740afa42e777cb369124c178225) | Not needed: custom keeps the URL scheme check, so on_link still goes through ui_open_url_link | test_text_viewer_links: define g_hMainWnd for on_link |

### Reverts on `main` that `custom` does not follow

These reverts put back the original's own behaviour: the lift had matched the original, and the
fix changed it on purpose. `custom` keeps the fixes. Two reverted fixes rested on a misreading
instead, and `custom` follows those: the whip shimmer (`fca55594`, read from the wrong routine) and
the NPC music restart (`874444c4`, which read the stream status code the wrong way round). The
quest journal search (`ff1a1f05`) is followed for the same reason: its fix searched one tab more
than the original.

| Revert on `main` | On custom | Change |
| --- | --- | --- |
| [dbba05ad6a](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/dbba05ad6afdab41a18a26fbd969303354d0b2ad) | Job 6017 is the first mercenary in the game data; custom also stops at 6046 | Revert "Count job 6017 as a mercenary" |
| [4bfd69ab22](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/4bfd69ab227c5c125747ff5c1820cb43980ca6df) | Only http and https links are opened; a hardening custom keeps | Revert "Open only http and https targets from text-viewer URL links" |
| [598fd4c531](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/598fd4c53144b6d84e0559aa6520a0f23f8f4fae) | The ESC list gets no entry for a window no factory makes | Revert "Stop requesting the nonexistent option window 0xd" |
| [165eb950a7](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/165eb950a7e5a96ac45f1a4897471f42044d0717) | Custom has its own single-height version of the fix | Revert "Keep a status window saved as a title bar minimized" |
| [b1b9fa0665](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/b1b9fa0665b7e63a2e8b336182abbd7738c9a863) | The hints follow the skill lists custom shows | Revert "Find prerequisite tab hints in the displayed skill lists" |
| [6832fcf74a](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/6832fcf74abb61755ba2efc906af0e41065fedef) | The navigation directory check uses its own search handle | Revert "Check the navigation directory with its own search handle" |
| [8ed1087779](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/8ed1087779831793ecc2a1693025e6566cc2778c) | Approved on purist (review ledger, guild flags) | Revert "Load guild flag animations through the normal cache path" |
| [0773244266](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/0773244266da5bc5929f7e68ad58a4de8cfc1f03) | Custom keeps both halves: the maximum level read under its own length, and the page buttons | Revert "Correct Adventurer Agency level and pagination controls" |
| [a3729f9bb2](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/a3729f9bb26154043e08d602b5c9c67a75327ff2) | Custom saves merchant presets locally and keeps the stricter match | Revert "Match item options and restore valid merchant preset entries" |
| [d714811b9f](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/d714811b9fb6e628589d815a395e83770686392a) | Goes with a3729f9b; the merchant restore test stays | Revert "Update merchant restore regression for compacted results" |
| [fa9f7f76d1](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/fa9f7f76d1afdc292bb50dc3e3302037bcf477b2) | A cleared macro is saved as cleared | Revert "Persist cleared chat macros as empty strings" |
| [77ea681cf6](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/77ea681cf6193df57fde8671056285828f98a5ff) | Not needed on custom, code removed (roulette) | Revert "Order roulette prizes by row then position" |

### Not ported

| Commit on `main` | Why | Change |
| --- | --- | --- |
| [70413bb9ff](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/70413bb9ffccccfef5803511923eebd83e088c49) | Custom keeps comments in its sources; main has a no-comment rule | Drop the comments that later commits added |
| [78a5cc82e3](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/78a5cc82e3732e39eb0d90284bd7ea0a9d7926a8) | Oracle tests: none passes on custom unchanged (below) | tests/oracle: the original executable as an in-process oracle |
| [472c3a5332](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/472c3a53325ce9889c858c34b2c11368ecd9b345) | Oracle tests | tests/oracle: the job, party and pet lookups against the original |
| [f5620d6b4f](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/f5620d6b4f6f3e1b73ff0e34bcff7df14d409233) | Oracle tests | tests/oracle: the ground-skill table against the original |
| [de451abfad](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/de451abfadd408050465915972766f6c7c325de4) | Oracle tests | tests/oracle: the skill-use handler against the original |
| [8c7e2c7e96](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/8c7e2c7e963a3e3247c5da36ce221a61e6840f06) | Oracle tests | tests/oracle: the skill-hit handler against the original, and the shared skill scaffolding |
| [f3fbb6e3c0](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/f3fbb6e3c0edbfe6304c775dba7e31fa932c99a8) | Oracle tests | tests/oracle: the cast-start handler against the original |
| [8be1ce97ea](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/8be1ce97ea4fbe2208f86b1e89319f54a3617e77) | Oracle tests | tests/oracle: the action manager's dropped cast arguments are read by no action |
| [031109f1c8](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/031109f1c871c30140c4b68484283cd2d1e53d35) | Parity harness: custom is not held to the reference client | tests/parity: the record-and-replay harness, and the client's state probe |
| [720189be19](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/720189be194e0e675148e171ca1da8bdf959d3e3) | Parity harness | tests/parity: first runs, and the original client in the harness |
| [681bb731ba](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/681bb731bac9444802b372ef6d43b88e2622ea3c) | README | README: Scroll Lock toggles the overlay only with RAGEXE_OVERLAY set |
| [123877dbf1](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/123877dbf15cb35f41714ba6996d0e9057d0ccd7) | README | README: list the emote cooldown rollover fix |
| [02a91ab50f](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/02a91ab50fae34c5d2a60ce73d37c802cc70a0db) | README | README: a short introduction, the full change list in docs/changes.md |
| [1a63e4a190](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/1a63e4a1901ca18ee0103b01e65dc8455ac4dbda) | README | README: the exe table without an empty header row |
| [7d4031889d](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/7d4031889d51a1c4eeb95b0eb181b5f443901a2b) | README | README: left-align the exe table's header row |
| [cf9e872d94](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/cf9e872d94655dabd58b37daa4a2c8f98f7e8179) | docs/changes.md lists main's changes from the original; custom's README lists its own | docs: where this tree is not the original |
| [35da545064](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/35da54506457d7cc6aa8a95af58540ce54087ae6) | docs/changes.md | docs: what this tree changes from the original, as a plain list |
| [bd74f7aad0](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/bd74f7aad04d6aa0edbabb993d9fc4e4b942ec73) | docs/changes.md | docs: one change list, docs/changes.md |
| [69385d1b97](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/69385d1b972bab054bb5a83aef73ab44a8701102) | AGENTS.md: custom has its own, from main's | AGENTS.md: how to work on this tree, from mistakes made on it |
| [b5e3ec71ec](https://github.com/HampterDev/2022-04-06-ragexe-in-c/commit/b5e3ec71ecbada1c8c289eab2b8d9b897d80c558) | AGENTS.md | AGENTS.md: only the rules that matter |

The oracle tests run the original's routines against the C over their whole input space. On
`custom` the job lookups extract functions that the third-class removal deleted, the skill-use
test needs the removed third- and fourth-class handler files, the cast test needs custom's
platform and mercenary links, and the ground-skill (776 of 22,528 calls) and skill-hit (every call)
tests mismatch. Each would need its stubs and the skill ranges custom removed taken into account;
that is left for its own change. `4232ffa0`, a further oracle test on the local `main`, was not
pushed yet and is not included.
