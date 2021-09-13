import xbmcaddon
import xbmcgui

addon = xbmcaddon.Addon()

xbmcgui.Dialog().textviewer(addon.getLocalizedString(30037), addon.getLocalizedString(30038))
