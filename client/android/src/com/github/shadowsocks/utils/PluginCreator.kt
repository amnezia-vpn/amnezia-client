package com.github.shadowsocks.utils

import com.github.shadowsocks.plugin.PluginOptions

class PluginCreator {
    companion object{
        fun getPluginString(id: String, options: String): String{
            return PluginOptions(id, options).toString(false)
        }
    }
}