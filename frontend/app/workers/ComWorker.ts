import * as Comlink from 'comlink';
import { io, Socket } from 'socket.io-client';
import Config from '@src/Config';

/*eslint-disable @typescript-eslint/no-explicit-any*/
export type ComWorkerAPI = {
	setupWebSockets(
		_eventNames: Array<string>,
		_handlerFunction: (data: any) => void,
		_errorHandlerFunction: (data: Error) => void,
		_connectHandlerFunction: () => void ): void;
	emitEvent(_eventName: string, _data: object): void;
};
/*eslint-enable @typescript-eslint/no-explicit-any*/
let socket: Socket | null = null;

const api: ComWorkerAPI = {
	setupWebSockets(eventNames, handlerFunction, errorHandlerFunction, connectHandlerFunction): void {
		socket = io(Config.API_URL);

		socket.on('connect', function() {
			console.warn('Connected to server');
			connectHandlerFunction();
			socket?.emit('getAlarmProfiles', { message: undefined });
		});

		socket.on('connect_error', (err: Error) => handleErrors(err));
		
		socket.on('connect_failed', (err: Error) => handleErrors(err));

		function handleErrors(err: Error) {
			console.error('Websocket Error: ' + err);
			errorHandlerFunction(err);
			setTimeout(() => {
				socket?.connect();
			}, 1000);
		};

		eventNames.forEach((eventName: string) => {
			socket?.on(eventName, (data: object): void => {handlerFunction({data: data, eventName: eventName})});
		});
	},
	emitEvent(eventName, data): void {
		socket?.emit(eventName, data);
	}
};

Comlink.expose(api);