# message/__init__.py
"""
Message protocol handling layer.
Provides message parsing, building, buffering, and validation.
"""

from .message_types import ClientMessageType, ServerMessageType
from .message_buffer import MessageBuffer
from .protocol_message import ProtocolMessage
from .message_protocol import MessageProtocol
from .validator import MessageValidator, ValidationError

__all__ = [
    'ClientMessageType',
    'ServerMessageType',
    'MessageBuffer',
    'ProtocolMessage',
    'MessageProtocol',
    'MessageValidator',
    'ValidationError',
]
