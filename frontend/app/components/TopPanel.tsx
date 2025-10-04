"use client";
import { useScrollThreshold } from "@components/useScrollThreshold";
import AlarmEnableButton from "@components/AlarmEnableButton"
import Panel from "@components/Panel"

const TopPanel: React.FC = ({}) => {
    const shrink = useScrollThreshold(10); 

    return (
        <Panel 
            padding={shrink ? "0px" : "10px"}
            alignItems={shrink ? "flex-start" : undefined}
            position={"fixed"}
            zIndex={10}
            hidebackground={!!shrink}
            minHeight={"50px"}
            className={`topPanel ${shrink ? "shrink" : ""}`}
        >
            <AlarmEnableButton showIcon={!shrink} className={`alarmButton ${shrink ? "shrink" : ""}`}></AlarmEnableButton>
        </Panel>
    );
};

export default TopPanel;