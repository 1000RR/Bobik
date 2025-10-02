import MjpegImage from "@components/MjpegImage";
import React from "react";
import Panel from "@components/Panel"

const SecurityVideos: React.FC<{
    className?: string
}> = ({ className }) => {
    return (
        <Panel padding={"0 10px 0 10px"} gap={0} rowGap={0} alignItems={"center"} flexDirection={"row"} className={className}>
            <MjpegImage src="https://bobik.lan/video/"></MjpegImage>
        </Panel>
    );
};

export default SecurityVideos;