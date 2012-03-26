# Copyright (c) 2008 Simon Busch
#
# This file is part of python-elementary.
#
# python-elementary is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# python-elementary is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with python-elementary.  If not, see <http://www.gnu.org/licenses/>.
#

from c_elementary import Object, Window, Background, Icon, \
    Box, Button, Web, Scroller, Label, Frame, Flip, Table, \
    Clock, Layout, Hover, Entry, Bubble, \
    Photo, Hoversel, Toolbar, ToolbarItem, List, ListItem, Slider, \
    Radio, Check, Naviframe, InnerWindow, Image, Spinner, Fileselector, FileselectorEntry, Notify, \
    Separator, Progressbar, Menu, MenuItem, MenuItemSeparator, Panel, \
    Genlist, GenlistItem, GenlistItemClass, WidgetItem, ScrolledEntry, \
    Gengrid, GengridItem, GengridItemClass, \
    init, shutdown, run, exit, scale_get, scale_set, \
    config_finger_size_get, config_finger_size_set, coords_finger_size_adjust, \
    theme_overlay_add, theme_extension_add, policy_set, policy_get, \
    tooltip_delay_set, tooltip_delay_get, cursor_engine_only_set, \
    cursor_engine_only_get, focus_highlight_enabled_get, \
    focus_highlight_enabled_set, focus_highlight_animate_get, \
    focus_highlight_animate_set, preferred_engine_get, preferred_engine_set, \
    engine_get, engine_set

ELM_WIN_BASIC = 0
ELM_WIN_DIALOG_BASIC = 1

ELM_WIN_KEYBOARD_UNKNOWN = 0
ELM_WIN_KEYBOARD_OFF = 1
ELM_WIN_KEYBOARD_ON = 2
ELM_WIN_KEYBOARD_ALPHA = 3
ELM_WIN_KEYBOARD_NUMERIC = 4
ELM_WIN_KEYBOARD_PIN = 5
ELM_WIN_KEYBOARD_PHONE_NUMBER = 6
ELM_WIN_KEYBOARD_HEX = 7
ELM_WIN_KEYBOARD_TERMINAL = 8
ELM_WIN_KEYBOARD_PASSWORD = 9
ELM_WIN_KEYBOARD_IP = 10
ELM_WIN_KEYBOARD_HOST = 11
ELM_WIN_KEYBOARD_FILE = 12
ELM_WIN_KEYBOARD_URL = 13
ELM_WIN_KEYBOARD_KEYPAD = 14
ELM_WIN_KEYBOARD_J2ME = 15

ELM_POLICY_QUIT = 0
ELM_POLICY_LAST = 1

ELM_POLICY_QUIT_NONE = 0
ELM_POLICY_QUIT_LAST_WINDOW_CLOSED = 1

ELM_HOVER_AXIS_NONE = 0
ELM_HOVER_AXIS_HORIZONTAL = 1
ELM_HOVER_AXIS_VERTICAL = 2
ELM_HOVER_AXIS_BOTH = 3

ELM_TEXT_FORMAT_PLAIN_UTF8 = 0
ELM_TEXT_FORMAT_MARKUP_UTF8 = 1

ELM_ICON_NONE = 0
ELM_ICON_FILE = 1
ELM_ICON_STANDARD = 2

ELM_SCROLLER_POLICY_AUTO = 0
ELM_SCROLLER_POLICY_ON = 1
ELM_SCROLLER_POLICY_OFF = 2

ELM_LIST_COMPRESS = 0
ELM_LIST_SCROLL = 1
ELM_LIST_LIMIT = 2

ELM_OBJECT_SELECT_MODE_DEFAULT = 0
ELM_OBJECT_SELECT_MODE_ALWAYS = 1
ELM_OBJECT_SELECT_MODE_NONE = 2
ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY = 3
ELM_OBJECT_SELECT_MODE_MAX = 4

ELM_NOTIFY_ORIENT_TOP = 0
ELM_NOTIFY_ORIENT_CENTER = 1
ELM_NOTIFY_ORIENT_BOTTOM = 2
ELM_NOTIFY_ORIENT_LEFT = 3
ELM_NOTIFY_ORIENT_RIGHT = 4
ELM_NOTIFY_ORIENT_TOP_LEFT = 5
ELM_NOTIFY_ORIENT_TOP_RIGHT = 6
ELM_NOTIFY_ORIENT_BOTTOM_LEFT = 7
ELM_NOTIFY_ORIENT_BOTTOM_RIGHT = 8

ELM_PANEL_ORIENT_TOP = 0
ELM_PANEL_ORIENT_BOTTOM = 1
ELM_PANEL_ORIENT_LEFT = 2
ELM_PANEL_ORIENT_RIGHT = 3

ELM_GENLIST_ITEM_NONE = 0
ELM_GENLIST_ITEM_TREE = 1
ELM_GENLIST_ITEM_GROUP = 2

ELM_FLIP_ROTATE_Y_CENTER_AXIS = 0
ELM_FLIP_ROTATE_X_CENTER_AXIS = 1
ELM_FLIP_ROTATE_XZ_CENTER_AXIS = 2
ELM_FLIP_ROTATE_YZ_CENTER_AXIS = 3
ELM_FLIP_CUBE_LEFT = 4
ELM_FLIP_CUBE_RIGHT = 5

ELM_BUBBLE_POS_TOP_LEFT = 0
ELM_BUBBLE_POS_TOP_RIGHT = 1
ELM_BUBBLE_POS_BOTTOM_LEFT = 2
ELM_BUBBLE_POS_BOTTOM_RIGHT = 3

ELM_WEB_ZOOM_MODE_MANUAL = 0
ELM_WEB_ZOOM_MODE_AUTO_FIT = 1
ELM_WEB_ZOOM_MODE_AUTO_FILL = 2

c_elementary.init()
