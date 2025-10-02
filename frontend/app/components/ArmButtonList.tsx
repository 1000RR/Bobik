"use client";
import React from "react";
import Panel from "@components/Panel"
import { useSelector } from "react-redux";
import { AlarmProfile, AppStateSlice, StatusResponse } from "./AppStateSlice";
import Button from "./Button";
import { emitDisarmEvent, emitArmAndChangeProfileEvent, emitChangeProfileEvent } from "@src/WebSocketService";

type AlarmProfileDescriptor = {
    id: number,
    name: string,
    enabled: boolean
};

export enum ArmButtonMode {
    SWITCH,
    SWITCH_AND_ENABLE,
    SWITCH_AND_ENABLE_QUICKLIST
}

const ArmButtonContainer: React.FC<{
    className?: string,
    buttonMode?: ArmButtonMode
}> = ({ buttonMode = ArmButtonMode.SWITCH }) => {

    const alarmProfilesToDisplay = useSelector(function (state: AppStateSlice) { 
        return state.appState.status.quickSetAlarmProfiles;
    });
    const alarmProfiles = useSelector(function (state: AppStateSlice) { 
        return state.appState.alarmProfiles.profiles;
    });
    const selectedProfileNumber = useSelector(function (state: AppStateSlice) { 
        return Number((state.appState.status as StatusResponse).profileNumber);
    });
    const alarmArmed = useSelector(function (state: AppStateSlice) { 
        return state.appState.status.armStatus === 'ARMED';
    });
    const generatedAlarmProfileList:Array<AlarmProfileDescriptor> = [ArmButtonMode.SWITCH_AND_ENABLE_QUICKLIST, ArmButtonMode.SWITCH_AND_ENABLE].includes(buttonMode) ? [{
        name: "⏻ Disarm",
        id: -1,
        enabled: !alarmArmed
    }] : [];

    if (buttonMode == ArmButtonMode.SWITCH_AND_ENABLE_QUICKLIST) { //traverse special profiles in order
        alarmProfilesToDisplay?.forEach((index: number) => {
            generatedAlarmProfileList.push({
                name: (`🔒 Arm ${alarmProfiles[index].name}`),
                id: index,
                enabled: alarmArmed && selectedProfileNumber === index
            });
        });
    } else {
        alarmProfiles?.forEach((alarmProfile: AlarmProfile, index: number) => {
            generatedAlarmProfileList.push({
                name: (`${[ArmButtonMode.SWITCH_AND_ENABLE].includes(buttonMode) && '🔒 Arm '}${index}: ${alarmProfile.name}`),
                id: index,
                enabled: selectedProfileNumber === index
            });
        });
    }

    const clickHandler:React.MouseEventHandler<HTMLButtonElement> = function(event) {
        const profileId =  Number.parseInt(event.currentTarget.id); //-1 is disable button manually added
        profileId === -1 ?  emitDisarmEvent() : (
            [ArmButtonMode.SWITCH_AND_ENABLE_QUICKLIST, ArmButtonMode.SWITCH_AND_ENABLE].includes(buttonMode) ? emitArmAndChangeProfileEvent(profileId, alarmArmed) : emitChangeProfileEvent(profileId)
        );
    };

    return (
        <Panel flexDirection="row" alignItems="center" gap={"10px"} rowGap={"10px"}>
            {generatedAlarmProfileList.map((alarmProfile: AlarmProfileDescriptor, index) => (
                <Button id={alarmProfile.id.toString()} key={index} onClick={clickHandler} className={(alarmProfile.enabled ? " alarm_button_enabled " : " alarm_button_disabled ") + " dimmable alarm_button"} >
                    {alarmProfile.name}
                </Button>
            ))}
        </Panel>
    );
};


const ArmButtonList: React.FC<{
    className?: string,
    buttonMode?: ArmButtonMode
}> = ({ buttonMode = ArmButtonMode.SWITCH }) => {
    return (
       <ArmButtonContainer buttonMode={buttonMode}></ArmButtonContainer>
    );
};

export default ArmButtonList;
 
 
