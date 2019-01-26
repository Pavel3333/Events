# -*- coding: utf-8 -*-
import imp
from xfw.constants import PATH

import re
import BigWorld
import BattleReplay

from Avatar import PlayerAvatar
from PlayerEvents import g_playerEvents
from constants import ARENA_PERIOD
from gui import InputHandler

from gui.Scaleform.daapi.view.lobby.LobbyView import LobbyView
from gui.Scaleform.genConsts.BATTLE_VIEW_ALIASES import BATTLE_VIEW_ALIASES
from gui.SystemMessages import SM_TYPE, pushMessage
from gui.shared import EVENT_BUS_SCOPE, events, g_eventBus
from notification.settings import NOTIFICATION_TYPE
from notification.actions_handlers import NotificationsActionsHandlers

#hooked
showed = False

def hookedLobby(func, *args):
    func(*args)
    global showed
    if not showed:
        error_code = trajectorymod.trj.e() #get_error_code
        txt_dial = ''
        if not error_code:
            txt_dial = u'<font size="14" color="#ffcc00">%s<a href="event:https://pavel3333.ru/trajectorymod">%s</a></font>'%(trajectorymod.config.i18n['UI_message_thx'], trajectorymod.config.i18n['UI_message_thx_2'])
        elif error_code in (2, 3, 4, 5):
            txt_dial = u'<font size="14" color="#ffcc00">%s</font>'%(trajectorymod.config.i18n['UI_err_%s'%(error_code)])
        elif error_code == 8:
            txt_dial = u'<font size="14" color="#ffcc00">%s. <a href="event:https://pavel3333.ru/youtube">%s</a></font>'%(trajectorymod.config.i18n['UI_message_deact'], trajectorymod.config.i18n['UI_message_deact_2'])
        else:
            txt_dial = u'<font size="14" color="#ffcc00">TrajectoryMod: Error %s</font>'%(error_code)
        pushMessage(txt_dial, SM_TYPE.GameGreeting)
        if not error_code and not trajectorymod.config.data['show_warning']:
            pushMessage(trajectorymod.config.i18n['UI_warning'], SM_TYPE.GameGreeting)
        showed = True

def hookedHandleAction(func, *args):
    if args[2] == NOTIFICATION_TYPE.MESSAGE and re.match('https?://', args[4], re.I):
        BigWorld.wg_openWebBrowser(args[4])
    else:
        func(*args)

def byteify(input):
    if isinstance(input, dict):
        return {byteify(key): byteify(value) for key, value in input.iteritems()}
    elif isinstance(input, unicode):
        return input.encode('utf-8')
    else:
        return input

#######

class Trajectory(object):
    def __init__(self):
        print '[TrajectoryMod]: loading module...'
        self.trj = imp.load_dynamic('trj', PATH.XFWLOADER_PACKAGES_REALFS + '/TrajectoryMod_FREE/native/trj.pyd')
        self.config = self.trj.l #g_config
        self.trj.g(self.template, self.apply, byteify) #init
        print '[TrajectoryMod]: loading module OK'
        
        self.isGetted = False
        
        self._setForcedGuiControlMode = PlayerAvatar.setForcedGuiControlMode
        
        g_playerEvents.onAccountShowGUI.__iadd__(self.trj.b) #check
        
        g_eventBus.addListener(events.ComponentEvent.COMPONENT_REGISTERED, self.onComponentRegistered, EVENT_BUS_SCOPE.GLOBAL)
        
        _PlayerAvatar__destroyGUI = PlayerAvatar._PlayerAvatar__destroyGUI
        PlayerAvatar._PlayerAvatar_destroyGUI = lambda *args: self.battleLeaving(_PlayerAvatar__destroyGUI, *args)
    
    def battleLoading(self):
        self.isGetted = False
        
        if self.trj.c() is None: #start
            self.isGetted = True
            
            InputHandler.g_instance.onKeyDown.__iadd__(self.inject_handle_key_event)
            
            PlayerAvatar.setForcedGuiControlMode = lambda *args: self.hook_setForcedGuiControlMode(self._setForcedGuiControlMode, *args)
            
            if self.config.data['hideMarkersInBattle']:
                BigWorld.player().arena.onPeriodChange.__iadd__(self.hideMarkersInBattle)
    
    def onComponentRegistered(self, event):
        if event.alias == BATTLE_VIEW_ALIASES.BATTLE_LOADING:
            self.battleLoading()
        if event.alias == 'crosshairPanel':
            self.trj.a() #battle_greetings
    
    def inject_handle_key_event(self, event):
        self.trj.h(event) #inject_handle_key_event
    
    def hook_setForcedGuiControlMode(self, func, *args):
        self.trj.k(args[1]) #hook_setForcedGuiControlMode
        func(*args)
    
    def hideMarkersInBattle(self, period, *args):
        if period == ARENA_PERIOD.BATTLE:
            self.trj.i() #hideMarkersInBattle

    def battleLeaving(self, func, *args):
        func(*args)
        
        self.trj.d() #fini
        if self.isGetted:
            PlayerAvatar.setForcedGuiControlMode = self._setForcedGuiControlMode
            InputHandler.g_instance.onKeyDown.__isub__(self.inject_handle_key_event)
            if self.config.data['hideMarkersInBattle']:
                BigWorld.player().arena.onPeriodChange.__isub__(self.hideMarkersInBattle)
    
    def template(self):
        return {
            'modDisplayName' : self.config.i18n['UI_description'],
            'settingsVersion': self.config.version_id,
            'enabled'        : self.config.data['enabled'],
            'column1'        : [{
                'type'        : 'HotKey',
                'text'        : self.config.i18n['UI_setting_buttonShow_text'],
                'tooltip'     : self.config.i18n['UI_setting_buttonShow_tooltip'],
                'value'       : self.config.data['buttonShow'],
                'defaultValue': self.config.buttons['buttonShow'],
                'varName'     : 'buttonShow'
            }, {
                'type'        : 'HotKey',
                'text'        : self.config.i18n['UI_setting_buttonMinimap_text'],
                'tooltip'     : self.config.i18n['UI_setting_buttonMinimap_tooltip'],
                'value'       : self.config.data['buttonMinimap'],
                'defaultValue': self.config.buttons['buttonMinimap'],
                'varName'     : 'buttonMinimap'
            }, {
                'type'   : 'CheckBox',
                'text'   : self.config.i18n['UI_setting_hideMarkersInBattle_text'],
                'value'  : self.config.data['hideMarkersInBattle'],
                'tooltip': self.config.i18n['UI_setting_hideMarkersInBattle_tooltip'],
                'varName': 'hideMarkersInBattle'
            }],
            'column2'        : [{
                'type'   : 'CheckBox',
                'text'   : self.config.i18n['UI_setting_showOnStartBattle_text'],
                'value'  : self.config.data['showOnStartBattle'],
                'tooltip': self.config.i18n['UI_setting_showOnStartBattle_tooltip'],
                'varName': 'showOnStartBattle'
            }, {
                'type'   : 'CheckBox',
                'text'   : self.config.i18n['UI_setting_showBattleGreetings_text'],
                'value'  : self.config.data['showBattleGreetings'],
                'tooltip': self.config.i18n['UI_setting_showBattleGreetings_tooltip'],
                'varName': 'showBattleGreetings'
            }, {
                'type'   : 'CheckBox',
                'text'   : self.config.i18n['UI_setting_show_warning_text'],
                'value'  : self.config.data['show_warning'],
                'tooltip': self.config.i18n['UI_setting_show_warning_tooltip'],
                'varName': 'show_warning'
            }, {
                'type'   : 'CheckBox',
                'text'   : self.config.i18n['UI_setting_createMarkers_text'],
                'value'  : self.config.data['createMarkers'],
                'tooltip': self.config.i18n['UI_setting_createMarkers_tooltip'],
                'varName': 'createMarkers'
            }, {
                'type'        : 'Dropdown',
                'text'        : self.config.i18n['UI_setting_Marker_text'],
                'tooltip'     : self.config.i18n['UI_setting_Marker_tooltip'],
                'itemRenderer': 'DropDownListItemRendererSound',
                'options'     : self.trj.f(), #generator_menu
                'width'       : 200,
                'value'       : self.config.data['marker'],
                'varName'     : 'marker'
            }
            ]
        }
    
    def apply(self, settings):
        from gui.mods.mod_mods_gui import g_gui
        self.config.data = g_gui.update_data(self.config.ids, settings, 'pavel3333')
        g_gui.update(self.config.ids, self.template)
    
if not BattleReplay.isPlaying():
    trajectorymod = Trajectory()
    
    #hooks
    handleAction_ = NotificationsActionsHandlers.handleAction
    NotificationsActionsHandlers.handleAction = lambda *args: hookedHandleAction(handleAction_, *args)
    
    populate_ = LobbyView._populate
    LobbyView._populate = lambda *args: hookedLobby(populate_, *args)
