

#角色
CREATE TABLE IF NOT EXISTS client_data  #很少或不更新的角色属性
(
    openid                  VARCHAR(32) NOT NULL ,   #openid
    cuid					BIGINT      NOT NULL ,   #角色id
    money1                  INT         NOT NULL DEFAULT 0,     #货币1
    money2                  INT         NOT NULL DEFAULT 0,     #货币2
    roomid                  INT         NOT NULL DEFAULT 0,     #roomid
    g13_history             BLOB,                               #13水比赛记录
    PRIMARY KEY(openid),
    KEY(cuid)
) CHARSET=utf8;

CREATE TABLE IF NOT EXISTS room_data  #经常更新的角色属性
(
    roomid			    	BIGINT      NOT NULL,               #房间id
    owner_cuid              BIGINT      NOT NULL,               #房主id
    game_type               INT         NOT NULL default 0,     #游戏类型
    g13_data				BLOB,                               #13水的数据
    PRIMARY KEY(roomid, owner_cuid)
) CHARSET=utf8;


