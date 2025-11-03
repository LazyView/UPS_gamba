"""
Card widget - displays a single playing card.

A reusable component for showing cards in hand, on the board, etc.
"""

from PyQt5.QtWidgets import QWidget, QLabel, QVBoxLayout
from PyQt5.QtCore import Qt, pyqtSignal, QSize
from PyQt5.QtGui import QPainter, QColor, QFont, QPen, QBrush

from game import Card


class CardWidget(QWidget):
    """
    Widget for displaying a playing card.
    
    Features:
    - Shows rank and suit with proper symbols
    - Clickable (emits clicked signal)
    - Multiple visual states (normal, selected, disabled, special)
    - Proper styling for different card types
    
    Signals:
        clicked: Emitted when card is clicked
    
    Responsibilities:
    - Display card visually
    - Handle click events
    - Show selected state
    
    Does NOT:
    - Validate card plays
    - Handle game logic
    """
    
    # Signals
    clicked = pyqtSignal(object)  # Emits the Card object
    
    # Suit symbols
    SUIT_SYMBOLS = {
        'H': '♥',  # Hearts
        'D': '♦',  # Diamonds
        'C': '♣',  # Clubs
        'S': '♠'   # Spades
    }
    
    # Suit colors
    SUIT_COLORS = {
        'H': '#e74c3c',  # Red
        'D': '#e74c3c',  # Red
        'C': '#2c3e50',  # Black
        'S': '#2c3e50'   # Black
    }
    
    # Special card colors
    SPECIAL_COLORS = {
        '2': '#9b59b6',   # Purple (wild)
        '7': '#e67e22',   # Orange (must play low)
        '10': '#e74c3c'   # Red (burn pile)
    }
    
    def __init__(self, card: Card, parent=None):
        """
        Initialize card widget.
        
        Args:
            card: Card object to display
            parent: Parent widget
        """
        super().__init__(parent)
        
        self.card = card
        self.is_selected = False
        self.is_clickable = True
        
        # Fixed size for cards
        self.setFixedSize(80, 110)
        
        # Enable mouse tracking
        self.setMouseTracking(True)
        
        # Cursor
        self.setCursor(Qt.PointingHandCursor)
        
        # Styling - thicker, darker border for visibility
        self.setStyleSheet("""
            CardWidget {
                background-color: white;
                border: 3px solid #2c3e50;
                border-radius: 8px;
            }
            CardWidget:hover {
                border: 3px solid #3498db;
            }
        """)
    
    def paintEvent(self, event):
        """Custom paint event to draw the card"""
        super().paintEvent(event)
        
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing)
        
        # Get colors
        suit_color = QColor(self.SUIT_COLORS.get(self.card.suit, '#2c3e50'))
        
        # Check if special card
        if self.card.is_special():
            bg_color = QColor(self.SPECIAL_COLORS.get(self.card.rank, '#ffffff'))
            bg_color.setAlpha(30)  # Light tint
            painter.fillRect(self.rect(), bg_color)
        
        # Draw selected overlay
        if self.is_selected:
            overlay = QColor('#3498db')
            overlay.setAlpha(50)
            painter.fillRect(self.rect(), overlay)
            
            # Draw selection border
            pen = QPen(QColor('#3498db'), 4)  # Thicker selection border
            painter.setPen(pen)
            painter.drawRoundedRect(3, 3, self.width()-6, self.height()-6, 8, 8)
        
        # Get suit symbol
        suit_symbol = self.SUIT_SYMBOLS.get(self.card.suit, self.card.suit)
        
        # Draw rank and suit at top-left
        painter.setPen(suit_color)
        font = QFont("Arial", 12, QFont.Bold)
        painter.setFont(font)
        painter.drawText(8, 20, f"{self.card.rank}{suit_symbol}")
        
        # Draw large suit symbol in center
        center_font = QFont("Arial", 36)
        painter.setFont(center_font)
        painter.drawText(
            self.rect(),
            Qt.AlignCenter,
            suit_symbol
        )
        
        # Draw rank and suit at bottom-right (rotated)
        painter.save()
        painter.translate(self.width(), self.height())
        painter.rotate(180)
        font = QFont("Arial", 12, QFont.Bold)
        painter.setFont(font)
        painter.drawText(8, 20, f"{self.card.rank}{suit_symbol}")
        painter.restore()
        
        # Draw special card indicator
        if self.card.is_special():
            painter.setPen(QColor('#e74c3c'))
            font = QFont("Arial", 8, QFont.Bold)
            painter.setFont(font)
            
            if self.card.is_wild():
                painter.drawText(self.rect().adjusted(0, self.height()-15, 0, 0), 
                               Qt.AlignCenter, "WILD")
            elif self.card.is_seven():
                painter.drawText(self.rect().adjusted(0, self.height()-15, 0, 0), 
                               Qt.AlignCenter, "≤7")
            elif self.card.is_ten():
                painter.drawText(self.rect().adjusted(0, self.height()-15, 0, 0), 
                               Qt.AlignCenter, "BURN")
    
    def mousePressEvent(self, event):
        """Handle mouse click"""
        if self.is_clickable and event.button() == Qt.LeftButton:
            self.clicked.emit(self.card)
    
    def set_selected(self, selected: bool):
        """
        Set card selected state.
        
        Args:
            selected: True if selected
        """
        self.is_selected = selected
        self.update()  # Trigger repaint
    
    def toggle_selected(self):
        """Toggle card selected state"""
        self.set_selected(not self.is_selected)
    
    def set_clickable(self, clickable: bool):
        """
        Set whether card is clickable.
        
        Args:
            clickable: True if clickable
        """
        self.is_clickable = clickable
        
        if clickable:
            self.setCursor(Qt.PointingHandCursor)
        else:
            self.setCursor(Qt.ArrowCursor)
        
        # Update styling
        if not clickable:
            self.setStyleSheet("""
                CardWidget {
                    background-color: #ecf0f1;
                    border: 3px solid #bdc3c7;
                    border-radius: 8px;
                }
            """)
        else:
            self.setStyleSheet("""
                CardWidget {
                    background-color: white;
                    border: 3px solid #2c3e50;
                    border-radius: 8px;
                }
                CardWidget:hover {
                    border: 3px solid #3498db;
                }
            """)
    
    def get_card(self) -> Card:
        """Get the card object"""
        return self.card
    
    def sizeHint(self):
        """Provide size hint"""
        return QSize(80, 110)
    
    def minimumSizeHint(self):
        """Provide minimum size hint"""
        return QSize(80, 110)


class EmptyCardSlot(QWidget):
    """
    Widget for displaying an empty card slot (placeholder).
    Used for showing reserve cards count or empty positions.
    """
    
    def __init__(self, label: str = "", parent=None):
        """
        Initialize empty card slot.
        
        Args:
            label: Text to show in slot
            parent: Parent widget
        """
        super().__init__(parent)
        
        self.label = label
        self.setFixedSize(80, 110)
        
        self.setStyleSheet("""
            EmptyCardSlot {
                background-color: #ecf0f1;
                border: 2px dashed #bdc3c7;
                border-radius: 8px;
            }
        """)
    
    def paintEvent(self, event):
        """Custom paint event"""
        super().paintEvent(event)
        
        if self.label:
            painter = QPainter(self)
            painter.setRenderHint(QPainter.Antialiasing)
            
            painter.setPen(QColor('#7f8c8d'))
            font = QFont("Arial", 10)
            painter.setFont(font)
            painter.drawText(self.rect(), Qt.AlignCenter, self.label)
    
    def set_label(self, label: str):
        """Update label text"""
        self.label = label
        self.update()
