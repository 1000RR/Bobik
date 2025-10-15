"use client";

import { initializeWebSocket } from "@src/WebSocketService";
import TopPanel from "@components/TopPanel";
import IndicatorPanel from "@components/IndicatorPanel";
import ButtonWithDrawer from "@components/ButtonWithDrawer";
import GarageDoorButton from "@/app/components/GarageDoorButton";
import UnavailableOverlay from "@components/UnavailableOverlay";
import ArmButtonList, { ArmButtonMode } from "@components/ArmButtonList";
import SpecialFunctions from "@components/SpecialFunctions";
import TopPanelSpacer from "@components/TopPanelSpacer";
import SecurityVideos from "@components/SecurityVideos";
import Button from "@components/Button";
import Image from "next/image";
import { useEffect, useRef, useState } from "react";
import { useSelector, useDispatch } from 'react-redux';
import { AppState, AppStateSlice } from "./AppStateSlice";
import BuildId from "@components/BuildId";
import ImageCacheLoader from "@components/ImageCacheLoader";
import { UIControls, NotificationController } from "@components/UIControls";
import ParseUtils from "@src/ParseUtils";

const AppView: React.FC = () => {
    const appState: AppState = useSelector((state: AppStateSlice) => state.appState); //appState is the name of the slice
    const dispatch = useDispatch();
    const notifRef = useRef<NotificationController>(null);

    const [anySensorTriggered, setAnySensorTriggered] = useState<string | false>(false);

    const deviceMap: Map<string, string> = new Map<string, string>();

    const [isFirstLoad, setIsFirstLoad] = useState<boolean>(true);

    useSelector(function (state: AppStateSlice) { 
        const memberDeviceReadable = state.appState.status.memberDevicesReadable;
        deviceMap.clear();
        memberDeviceReadable.map((device: string) => {
            const id = ParseUtils.getDeviceIdFromDescriptor(device);
            const name = ParseUtils.getDeviceNameFromDescriptor(device);
            deviceMap.set(id, name);
        });
    });

    useEffect(() => {
        setAnySensorTriggered(
            appState.status.currentTriggeredDevices.length > 0 
            ? appState.status.currentTriggeredDevices.reduce(
                function (accumulator: string, currentValue: string, currentIndex: number) {
                    const deviceName = deviceMap.get(currentValue) || currentValue;
                    return accumulator + (currentIndex > 0 ? ", " : "") + deviceName;
                },
                '') 
            : false);
    }, [appState.status.currentTriggeredDevices]);

    useEffect(() => {
        const terminateWebSocket = initializeWebSocket(dispatch);
        return () => {
            terminateWebSocket();
        };
    }, [dispatch]);

    const serviceAvailable = appState.isConnected && !appState.isError && appState.isLoaded;
    const alarmTriggered = appState.status.alarmStatus === 'ALARM';

    const unavailableContent = <div style={{display: "flex", flexDirection: "column", alignItems: "center", fontSize: "40px", textAlign: "center", fontWeight: "normal", fontFamily: "futura"}}>
        Alarm Service Unavailable
        <Image className="fadeoutImageRound" src={"/assets/dogsleep.jpg"} width="150" height="150" alt=""></Image>
    </div>;
    const loadingContent = <div style={{display: "flex", flexDirection: "column", alignItems: "center", fontSize: "40px", textAlign: "center", fontWeight: "normal", fontFamily: "futura"}}>
        Loading Alarm
        <Image className="fadeoutImageRound" src={"/assets/dogread.jpg"} width="150" height="150" alt=""></Image>
    </div>;   
    
    useEffect(() => {
        if (anySensorTriggered) {
            notifRef.current?.sendNotification(
                `${ParseUtils.formatDate(new Date())}`,
                `Triggered: ${anySensorTriggered}`,
                'sensor trigger',
                true
            );
        }
    }, [anySensorTriggered]);
    
    useEffect(() => {
        const currentStatus = appState.status.armStatus;
        if (currentStatus !== appState.priorStatus?.armStatus) {
            notifRef.current?.sendNotification(
                `${ParseUtils.formatDate(new Date())}`,
                `Alarm ${currentStatus}${currentStatus === 'ARMED' ? " : " + appState.status.profile : ""}`,
                'arm status',
                true
            );
        }
    }, [appState.status.armStatus]); //priorStatus not included because change in status is a sufficient trigger

    useEffect(() => {
        if (alarmTriggered) {
            notifRef.current?.playAlarmSound();
        } else {
            notifRef.current?.stopAlarmSound();
        }
    }, [alarmTriggered]);

    const scrollToTop = (elName: string) => {
        const el = document.getElementById(elName);
        if (el) {
            el.scrollTo({
                top: 0,
                behavior: 'smooth'
            });
        }
    };

    const scrollToBottom = (elName: string) => {
        const el = document.getElementById(elName);
        if (el) {
            el?.scrollTo({
                top: el.scrollHeight,
                behavior: 'smooth'
            });
        }
    };

    return (
        <>
            {isFirstLoad ? <ImageCacheLoader
                urls={['/icon192.png',
                        '/icon512.png',
                        '/icon180.png',
                        '/favicon.ico',
                        '/assets/attackdog.jpg',
                        '/assets/dogread.jpg',
                        '/assets/dogsleep.jpg',
                        '/assets/dogue.jpg',
                        '/assets/garage_closed.png',
                        '/assets/garage_open.png',
                        '/assets/required.svg']}
                onReady={(map) => {
                    // Files are already loaded & decoded at this point
                    console.log("Assets preloaded:", map);
                    // You could stash them in a global store, or ignore them
                    setIsFirstLoad(false);
                }}
            /> : <></>}
            { serviceAvailable ? 
                <div style={{overflowX: 'hidden'}} className={`background ${alarmTriggered ? " blinkingTransitions " : " "}`}>
                    <TopPanel></TopPanel>
                    <TopPanelSpacer></TopPanelSpacer>
                    <IndicatorPanel></IndicatorPanel>
                    <ButtonWithDrawer flexDirection="column" className={appState.status?.armStatus === 'ARMED' ? "alarm-state-on" : "alarm-state-off"} buttonText="Alarm Control"><ArmButtonList buttonMode={ArmButtonMode.SWITCH_AND_ENABLE_QUICKLIST}></ArmButtonList></ButtonWithDrawer>
                    <ButtonWithDrawer flexDirection="row" buttonText="UI & Alert Controls" keepChildrenInDomOnClose={true}>
                        <UIControls ref={notifRef}/>
                        {/* keep at top level - must always be rendered as local alarm depends on elements being in DOM */}
                    </ButtonWithDrawer>
                    <ButtonWithDrawer flexDirection="column" justifyContent="flex-start" buttonText="Advanced" disableinternalspacing={true}>
                        <ButtonWithDrawer flexDirection="column" buttonText="Special Functions"><SpecialFunctions></SpecialFunctions></ButtonWithDrawer>
                        <ButtonWithDrawer flexDirection="column" buttonText="Extended Alarm Profiles List"><ArmButtonList buttonMode={ArmButtonMode.SWITCH_AND_ENABLE}></ArmButtonList></ButtonWithDrawer>
                        <ButtonWithDrawer flexDirection="column" justifyContent="flex-start" buttonText="Status" containsScrollable>
                            <div style={{display: "flex", gap: 10}}>
                                <Button onClick={() => {scrollToBottom("statusContainer")}} className="scrollToBottomBtn scroll-btn">Bottom</Button>
                                <Button onClick={() => {scrollToTop("statusContainer")}}  className="scrollToTopBtn scroll-btn">Top</Button>
                            </div>
                            <pre id="statusContainer" className="dimmable">{JSON.stringify(appState.status, null, 2)}</pre>
                        </ButtonWithDrawer>
                        <ButtonWithDrawer flexDirection="column" justifyContent="flex-start" buttonText="Past Events" containsScrollable>
                            <div style={{display: "flex", gap: 10}}>
                                <Button onClick={() => {scrollToBottom("eventsContainer")}} className="scrollToBottomBtn scroll-btn">Bottom</Button>
                                <Button onClick={() => {scrollToTop("eventsContainer")}}  className="scrollToTopBtn scroll-btn">Top</Button>
                            </div>
                            <pre id="eventsContainer" className="dimmable">{JSON.stringify(appState.pastEvents, null, 2)}</pre>
                        </ButtonWithDrawer>
                        <ButtonWithDrawer flexDirection="column" justifyContent="flex-start" buttonText="Profile Definitions" containsScrollable>
                            <div style={{display: "flex", gap: 10}}>
                                <Button onClick={() => {scrollToBottom("profilesContainer")}} className="scrollToBottomBtn scroll-btn">Bottom</Button>
                                <Button onClick={() => {scrollToTop("profilesContainer")}}  className="scrollToTopBtn scroll-btn">Top</Button>
                            </div>
                            <pre id="profilesContainer" className="dimmable">{JSON.stringify(appState.alarmProfiles, null, 2)}</pre>
                        </ButtonWithDrawer>
                        <ButtonWithDrawer flexDirection="row" buttonText="Garage Door"><GarageDoorButton margin="10px"></GarageDoorButton></ButtonWithDrawer>
                    </ButtonWithDrawer>
                    <ButtonWithDrawer flexDirection="row" buttonText="Security Video Stream" isOpen={true}><SecurityVideos></SecurityVideos></ButtonWithDrawer>
                    <BuildId></BuildId>
                </div> :
                <div>
                    <UnavailableOverlay>
                        {appState.isError && !appState.isConnected ? unavailableContent : loadingContent}
                        <ButtonWithDrawer disableinternalspacing={true} flexDirection="row" buttonText="Security Video Stream" isOpen={true}>
                            <SecurityVideos></SecurityVideos>
                        </ButtonWithDrawer>
                    </UnavailableOverlay>
                </div>
            }
        </>
    );
};

export default AppView;