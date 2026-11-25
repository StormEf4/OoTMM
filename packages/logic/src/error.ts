export class LogicError extends Error {
  constructor(message: string) {
    super(message);
    this.name = "LogicError";
  }
};

export class LogicEntranceError extends LogicError {
  constructor(message: string) {
    super(message);
    this.name = "LogicEntranceError";
  }
};

export class LogicSeedError extends LogicError {
  constructor(message: string) {
    super(message);
    this.name = "LogicSeedError";
  }
};

/* Fork: a logic pass kept failing while the enemy shuffle is on. The seed's
 * enemies are usually the cause (an enemy the item pool can't beat), so the
 * pipeline rerolls the whole enemy plan instead of retrying the same world. */
export class EnemyRerollError extends Error {
  constructor(message: string) {
    super(message);
    this.name = 'EnemyRerollError';
  }
}

export class EnemyCompatibilityError extends Error {
  constructor(readonly failures: import('./pathfind').EnemyCompatibilityFailure[], readonly worlds: import('./world').World[]) {
    super('Enemy replacements are incompatible with All Locations logic');
    this.name = 'EnemyCompatibilityError';
  }
}
