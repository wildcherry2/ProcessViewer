export type PID = number;

// Events that the native side can send to the JS side to update the process list or notify of changes.
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

// Object that is send alongside PROCESS_TITLEUPDATE events that contains the PID of the process whose title was 
// updated and the new title (if it exists, since not all processes have titles).
export interface ProcessInfoTitleUpdate {
    readonly pid: PID;
    readonly title?: string;
}

// Event map for ProcessInfoEvents and their data.
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

// Helper type to get the data type for a given ProcessInfoEvent, used for typing the data field of ProcessInfoUpdate objects.
export type DataFor<T extends ProcessInfoEvent> = ProcessInfoEventMap[T];

// Object that the native side sends to the JS side to update the process list. 
// Contains an event field that specifies the type of update and a data field that contains the relevant data for the update, 
// which is typed based on the event field.
export interface ProcessInfoUpdate<T extends ProcessInfoEvent> {
    readonly event: T;
    readonly data: DataFor<T>;
}

// Type for any ProcessInfoUpdate regardless of the event type, used for typing the arguments of ProcessTable's updateTable method.
export type AnyProcessInfoUpdate = {
    [K in ProcessInfoEvent]: ProcessInfoUpdate<K>;
}[ProcessInfoEvent];

// The ProcessInfo type that represents the information about a process that the native side sends to the JS side.
export interface ProcessInfo {
    readonly pid: PID;
    readonly name: string;
    readonly image?: string; // Base64 encoded image data for the process' icon.
    hidden?: boolean; // Whether the process is hidden by the search filter. This is updated by the native side through ProcessInfoEvents for multithreaded filtering.
    readonly path: string; // The path to the process executable, if the system provides it.
    readonly name_lc: string; // Lowercased version of the process name
    title?: string; // The process' title, if it has one and if the system provides it. This is updated by the native side through ProcessInfoEvents since window titles can change.
}

// The shape of the detail object for PTSelectionChangedEvents, which are sent when the selected process in the table changes.
export interface PTSelectionChangedEventDetails {
    readonly new_selection?: ProcessInfo;
}

export type PTSelectionChangedEvent = CustomEvent<PTSelectionChangedEventDetails>;
