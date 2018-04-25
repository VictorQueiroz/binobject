export abstract class CustomTypeProcessor<T> {
    abstract encode(value: T): Buffer;
    abstract decode(buffer: Buffer): T;
    abstract validate(value: T): boolean;
}

export interface CustomInstruction<T> {
    value: number;
    processor: CustomTypeProcessor<T>;
}
