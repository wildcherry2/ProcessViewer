
export class WeakArray<T extends object> {

    push(item: T) {
        this._refs.push(new WeakRef(item));
    }

    remove(item: T) {
        // compact the array while removing the target object
        let write = 0;
        for (let read = 0; read < this._refs.length; read++) {
            const current = this._refs[read].deref();
            if (current === undefined || current === item) {
                // skip dead refs and the object to remove
                continue;
            }
            this._refs[write++] = this._refs[read];
        }
        this._refs.length = write;
    }

    get size() {
        return this._refs.filter(ref => ref.deref() !== undefined).length;
    }

    *[Symbol.iterator]() {
        let write = 0; // write pointer for live references

        for (let read = 0; read < this._refs.length; read++) {
            const obj = this._refs[read].deref();
            if (obj !== undefined) {
                yield obj;                     // yield live object
                this._refs[write++] = this._refs[read]; // keep live ref
            }
        // dead refs are skipped and overwritten by future live refs
        }

        // truncate array to remove dead references at the end
        this._refs.length = write;
    }

    private _refs: WeakRef<T>[] = [];
}