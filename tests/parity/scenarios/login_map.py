"""Start-up to the map: Select Service, the login (the parity server's GM account paritytest),
the character selection, entering the map. Frame numbers assume the client paced at 16 ms
per frame."""

STEPS = [
    (120, 'capture', 'service_select'),
    (130, 'key', 'enter'),
    (200, 'capture', 'login_window'),
    (210, 'text', 'paritytest'),
    (230, 'key', 'tab'),
    (240, 'text', 'paritytest1'),
    (260, 'capture', 'login_typed'),
    (270, 'key', 'enter'),
    (430, 'capture', 'char_select'),
    (440, 'click', (910, 545)),
    (700, 'capture', 'map_700'),
    (1000, 'capture', 'map_1000'),
    (1300, 'capture', 'map_1300'),
    (1310, 'quit', None),
]
