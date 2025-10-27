export type PID = number;
export const enum ProcessInfoEvent {
    PROCESS_ADDED,// data is ProcessInfo
    PROCESS_REMOVED,// data is PID
    PROCESS_HIDDEN,// data is PID[]
    PROCESS_UNHIDDEN,// data is PID[]
    PROCESS_SHOWALL,// data is undefined
    PROCESS_HIDEALL,// data is undefined
    PROCESS_TITLEUPDATE,// data is ProcessInfoTitleUpdate,
    PROCESS_DARKMODECHANGE// data is boolean
}

export interface ProcessInfoTitleUpdate {
    readonly pid: PID;
    readonly title?: string;
}

export interface ProcessInfoEventMap {
    [ProcessInfoEvent.PROCESS_ADDED]: ProcessInfo;
    [ProcessInfoEvent.PROCESS_REMOVED]: PID;
    [ProcessInfoEvent.PROCESS_HIDDEN]: PID[];
    [ProcessInfoEvent.PROCESS_UNHIDDEN]: PID[];
    [ProcessInfoEvent.PROCESS_SHOWALL]: undefined;
    [ProcessInfoEvent.PROCESS_HIDEALL]: undefined;
    [ProcessInfoEvent.PROCESS_TITLEUPDATE]: ProcessInfoTitleUpdate;
    [ProcessInfoEvent.PROCESS_DARKMODECHANGE]: boolean;
}

export type DataFor<T extends ProcessInfoEvent> = ProcessInfoEventMap[T];

export interface ProcessInfoUpdate<T extends ProcessInfoEvent> {
    readonly event: T;
    readonly data: DataFor<T>;
}

export type AnyProcessInfoUpdate = {
    [K in ProcessInfoEvent]: ProcessInfoUpdate<K>;
}[ProcessInfoEvent];

export interface ProcessInfo {
    readonly pid: PID;
    readonly name: string;
    readonly image?: string;
    hidden?: boolean;
    readonly path: string;
    readonly name_lc: string;
    title?: string;
}

export interface PTSelectionChangedEventDetails {
    readonly new_selection?: ProcessInfo;
}

export type PTSelectionChangedEvent = CustomEvent<PTSelectionChangedEventDetails>;
