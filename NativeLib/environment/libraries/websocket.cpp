#include "../environment.hpp"

#include <lua.h>
#include <lualib.h>

#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>

// this isnt written by me - nop

class exploit_websocket {
public:
    lua_State* th = nullptr;
    bool connected = false;
    ix::WebSocket* webSocket;
    int onMessageRef;
    int onCloseRef;
    int threadRef;

    auto fireMessage(std::string message) -> void {
        if (!connected)
            return;

        lua_getref(th, onMessageRef);
        lua_getfield(th, -1, "Fire");
        lua_getref(th, onMessageRef);
        lua_pushlstring(th, message.c_str( ), message.size( ));
        lua_pcall(th, 2, 0, 0);
        lua_settop(th, 0);
    }

    auto fireClose( ) -> void {
        if (!connected)
            return;

        connected = false;

        lua_getref(th, onCloseRef);
        lua_getfield(th, -1, "Fire");
        lua_getref(th, onCloseRef);
        lua_pcall(th, 1, 0, 0);
        lua_settop(th, 0);

        lua_unref(th, onMessageRef);
        lua_unref(th, onCloseRef);
        lua_unref(th, threadRef);
    }

    int handleIndex(lua_State* ls)
    {
        luaL_checktype(ls, 1, LUA_TUSERDATA);
        if (!connected)
            return 0;
        
        std::string idx = luaL_checkstring(ls, 2);

        if (idx == "OnMessage") {
            lua_getref(ls, this->onMessageRef);
            lua_getfield(ls, -1, "Event");
            return 1;
        }
        else if (idx == "OnClose") {
            lua_getref(ls, this->onCloseRef);
            lua_getfield(ls, -1, "Event");
            return 1;
        }
        else if (idx == "Send") {
            lua_pushvalue(ls, -10003);
            lua_pushcclosure(ls,
                [ ](lua_State* L) -> int {
                    luaL_checktype(L, 1, LUA_TUSERDATA);
                    std::string data = luaL_checkstring(L, 2);
                    
                    exploit_websocket* webSocket = reinterpret_cast<exploit_websocket*>(lua_touserdata(L, -10003));
                    webSocket->webSocket->send(data, true);
                    return 0;
                }, ("WebSocket.Send"), 1);

            return 1;
        }
        else if (idx == "Close") {
            lua_pushvalue(ls, -10003);
            lua_pushcclosure(ls,
                [ ](lua_State* L) -> int {
                    luaL_checktype(L, 1, LUA_TUSERDATA);

                    exploit_websocket* webSocket = reinterpret_cast<exploit_websocket*>(lua_touserdata(L, -10003));
                    webSocket->webSocket->close( );
                    return 0;
                }, ("WebSocket.Close"), 1);
            return 1;
        }
        return 0;
    };
};

static int websocket_connect(lua_State* ls)
{
    LOGD(" LuauEnvCall -> websocket_connect - CallingThread -> %p", ls);
    
    std::string url = luaL_checkstring(ls, 1);
    exploit_websocket* webSocket = (exploit_websocket*)lua_newuserdata(ls, sizeof(exploit_websocket));
    *webSocket = exploit_websocket{ };

    webSocket->th = lua_newthread(ls);
    webSocket->threadRef = lua_ref(ls, -1);
    webSocket->webSocket = new ix::WebSocket( );
    webSocket->webSocket->setUrl(url);
    lua_pop(ls, 1);

    lua_getglobal(ls, "Instance");
    lua_getfield(ls, -1, "new");
    lua_pushstring(ls, "BindableEvent");
    lua_pcall(ls, 1, 1, 0);
    webSocket->onMessageRef = lua_ref(ls, -1);
    lua_pop(ls, 2);

    lua_getglobal(ls, "Instance");
    lua_getfield(ls, -1, "new");
    lua_pushstring(ls, "BindableEvent");
    lua_pcall(ls, 1, 1, 0);
    webSocket->onCloseRef = lua_ref(ls, -1);
    lua_pop(ls, 2);

    webSocket->webSocket->setOnMessageCallback(
        [webSocket](const ix::WebSocketMessagePtr& msg) -> void {
            if (msg->type == ix::WebSocketMessageType::Message) {
                webSocket->fireMessage(msg->str);
            }
            else if (msg->type == ix::WebSocketMessageType::Close || msg->type == ix::WebSocketMessageType::Error) {
                webSocket->fireClose( );
            }
            return;
        }
    );

    lua_newtable(ls); // metatable
    lua_pushstring(ls, "WebSocket");
    lua_setfield(ls, -2, "__type");

    lua_pushvalue(ls, -2);
    lua_pushcclosure(ls,
        [ ](lua_State* L) -> int {
            exploit_websocket* webSocket = reinterpret_cast<exploit_websocket*>(lua_touserdata(L, lua_upvalueindex(1)));
            return webSocket->handleIndex(L);
        },
    ("__index"), 1);
    lua_setfield(ls, -2, "__index");
    lua_setmetatable(ls, -2);

    webSocket->webSocket->connect(5);

    if (webSocket->webSocket->getReadyState( ) != ix::ReadyState::Open) {
        luaL_error(ls, "WebSocket connection failed");
    }
    
    webSocket->connected = true;
    webSocket->webSocket->start( );
    return 1;
}

static const luaL_Reg funcs[ ] = {
    {"connect", websocket_connect},
    {"Connect", websocket_connect},
    
    {nullptr, nullptr}
};

auto exploit::environment::websocket( lua_State* ls ) -> void
{
    lua_pushvalue(ls, LUA_GLOBALSINDEX);
    register_lib(ls, "WebSocket", funcs);
    lua_pop(ls, 1);
}