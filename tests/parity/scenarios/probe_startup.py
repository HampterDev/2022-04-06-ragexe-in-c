"""The state probe at start-up, offline: Select Service, then the login window."""

STEPS = [
    (120, 'capture', 'service_select'),
    (120, 'state', 'service_select'),
    (130, 'key', 'enter'),
    (200, 'capture', 'login_window'),
    (200, 'state', 'login_window'),
    (210, 'quit', None),
]
