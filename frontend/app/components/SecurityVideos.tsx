import MjpegImage from "@components/MjpegImage";
import React from "react";
import Panel from "@components/Panel"
import Config from "@src/Config";

const SecurityVideos: React.FC<{
    className?: string
}> = ({ className }) => {
    return (
        <Panel padding={"0 10px 0 10px"} gap={"0px"} rowGap={"0px"} alignItems={"center"} flexDirection={"row"} className={className}>
            <>
            {
                Config.VIDEO_URLS.map((url) => (
                    <MjpegImage key={url} src={url} />
                ))}
            </>
        </Panel>
    );
};

export default SecurityVideos;